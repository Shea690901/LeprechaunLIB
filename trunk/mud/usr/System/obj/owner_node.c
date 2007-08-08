# include <status.h>
# include <type.h>
# include <kernel/kernel.h>
# include <system/assert.h>
# include <system/object.h>

object   driver_;            /* driver object */
object   objectd_;           /* object manager */
int      uid_;               /* UID of owner */

mapping  pdb_entries_;       /* ([ int oid: mixed *entry ]) */
mapping  pdb_paths_;         /* ([ string path: ({ int oid, ... }) ]) */

int      next_index_;        /* index for the next managed LWO */
mapping  persistent_oids_;   /* ([ int oid: object obj ]) */
mapping  lightweight_oids_;  /* ([ bucket_index: ([ oid: environment ]) ]) */

/*
 * NAME:        create()
 * DESCRIPTION: initialize owner node
 */
static void create(int clone)
{
    if (clone) {
        driver_ = find_object(DRIVER);
        objectd_ = find_object(OBJECTD);
        uid_ = objectd_->query_uid(query_owner());

        pdb_entries_ = ([ ]);
        pdb_paths_ = ([ ]);

	next_index_ = 1;
        persistent_oids_ = ([ ]);
        lightweight_oids_ = ([ ]);
    }
}

private void link_child(int child_oid, mixed *child_entry, string parent_name)
{
    int      parent_uid, index, parent_oid;
    string   creator;
    mixed   *parent_entry;

    /* get program entry for parent */
    creator = driver_->creator(parent_name);
    parent_uid = objectd_->query_uid(creator);
    index = status(parent_name)[O_INDEX] + 1;
    parent_oid = OID_MASTER | (parent_uid << OID_OWNER_OFFSET)
        | (index << OID_INDEX_OFFSET);
    parent_entry = objectd_->query_program(parent_oid);

    /* link child */
    if (!parent_entry[PDB_CHILDREN]) {
        /* first child: create chain */
        parent_entry[PDB_CHILDREN] = child_oid;
        child_entry[PDB_PREVIOUS][parent_oid] = child_oid;
        child_entry[PDB_NEXT][parent_oid] = child_oid;
    } else {
        int     first_oid, previous_oid;
        mixed  *first_entry, *previous_entry;
        
        /* find links */
        first_oid = parent_entry[PDB_CHILDREN];
        first_entry = objectd_->query_program(first_oid);
        previous_oid = first_entry[PDB_PREVIOUS][parent_oid];
        previous_entry = objectd_->query_program(previous_oid);

        /* add new child to chain */
        previous_entry[PDB_NEXT][parent_oid] = child_oid;
        child_entry[PDB_PREVIOUS][parent_oid] = previous_oid;
        first_entry[PDB_PREVIOUS][parent_oid] = child_oid;
        child_entry[PDB_NEXT][parent_oid] = first_oid;
    }
}

private void unlink_child(int child_oid, mixed *child_entry, int parent_oid)
{
    int     previous_oid;
    mixed  *parent_entry;

    parent_entry = objectd_->query_program(parent_oid);
    previous_oid = child_entry[PDB_PREVIOUS][parent_oid];
    if (child_oid == previous_oid) {
        /* only link: remove chain */
        parent_entry[PDB_CHILDREN] = 0;
        child_entry[PDB_PREVIOUS][parent_oid] = nil;
        child_entry[PDB_NEXT][parent_oid] = nil;
    } else {
        int     next_oid;
        mixed  *previous_entry, *next_entry;

        /* unlink child */
        next_oid = child_entry[PDB_NEXT][parent_oid];
        previous_entry = objectd_->query_program(previous_oid);
        next_entry = objectd_->query_program(next_oid);

        previous_entry[PDB_NEXT][parent_oid] = next_oid;
        child_entry[PDB_PREVIOUS][parent_oid] = nil;
        next_entry[PDB_PREVIOUS][parent_oid] = previous_oid;
        child_entry[PDB_NEXT][parent_oid] = nil;

        if (parent_entry[PDB_CHILDREN] == child_oid) {
            /* child was first in chain: appoint next child instead */
            parent_entry[PDB_CHILDREN] = next_oid;
        }
    }
}

/*
 * NAME:        compile()
 * DESCRIPTION: the given object has just been compiled
 */
void compile(mixed obj, string *source, string *inherited)
{
    string   path;
    int      index, oid, i, size;
    mixed   *entry;

    ASSERT_ACCESS(previous_object() == objectd_);
    path = (typeof(obj) == T_STRING) ? obj : object_name(obj);

    if (path != DRIVER && path != AUTO && !sizeof(inherited)) {
        inherited = ({ AUTO });
    }

    index = status(path)[O_INDEX] + 1;
    oid = OID_MASTER | (uid_ << OID_OWNER_OFFSET)
        | (index << OID_INDEX_OFFSET);
    entry = pdb_entries_[oid];
    if (entry) {
        int *parent_oids;

        parent_oids = map_indices(entry[PDB_PREVIOUS]);
        size = sizeof(parent_oids);
        for (i = 0; i < size; ++i) {
            unlink_child(oid, entry, parent_oids[i]);
        }
    } else {
        entry = pdb_entries_[oid] = ({ path, ([ ]), ([ ]), 0 });
    }
    size = sizeof(inherited);
    for (i = 0; i < size; ++i) {
        link_child(oid, entry, inherited[i]);
    }

    if (pdb_paths_[path]) {
        pdb_paths_[path] += ({ oid });
    } else {
        pdb_paths_[path] = ({ oid });
    }
}

void clone(object obj)
{
    int index, oid;

    ASSERT_ACCESS(previous_object() == objectd_);
    sscanf(object_name(obj), "%*s#%d", index);
    oid = OID_CLONE | (uid_ << OID_OWNER_OFFSET) | (index << OID_INDEX_OFFSET);
    persistent_oids_[oid] = obj;
}

void destruct(object obj)
{
    int category, index, oid;

    ASSERT_ACCESS(previous_object() == objectd_);
    if (sscanf(object_name(obj), "%*s#%d", index)) {
        category = OID_CLONE;
    } else {
        category = OID_MASTER;
        index = status(obj)[O_INDEX] + 1;
    }
    oid = category | (uid_ << OID_OWNER_OFFSET) | (index << OID_INDEX_OFFSET);
    persistent_oids_[oid] = nil;
}

/*
 * NAME:        find()
 * DESCRIPTION: find a persistent object or managed LWO by number
 */
object find(int oid)
{
    ASSERT_ACCESS(previous_object() == objectd_);
    if ((oid & OID_CATEGORY_MASK) == OID_LIGHTWEIGHT) {
        int      index;
        mapping  bucket;
        object   environment;

        index = (oid & OID_INDEX_MASK) >> OID_INDEX_OFFSET;
        bucket = lightweight_oids_[(index - 1) / DATA_BUCKET_SIZE];
        if (!bucket) {
            return nil;
        }
        environment = bucket[oid];
        return environment ? environment->_F_find(oid) : nil;
    } else {
        return persistent_oids_[oid];
    }
}

/*
 * NAME:        remove_program()
 * DESCRIPTION: the last reference to the given program has been removed
 */
void remove_program(string path, int index)
{
    int      oid, i, size, *parent_oids;
    mixed   *entry;

    ASSERT_ACCESS(previous_object() == objectd_);
    ++index; /* use 1-based indices, not 0-based as supplied by DGD */
    oid = OID_MASTER | (uid_ << OID_OWNER_OFFSET)
        | (index << OID_INDEX_OFFSET);
    entry = pdb_entries_[oid];

    pdb_paths_[path] -= ({ oid });
    if (!sizeof(pdb_paths_[path])) {
        pdb_paths_[path] = nil;
    }

    parent_oids = map_indices(entry[PDB_PREVIOUS]);
    size = sizeof(parent_oids);
    for (i = 0; i < size; ++i) {
        unlink_child(oid, entry, parent_oids[i]);
    }
    pdb_entries_[oid] = nil;
}

/*
 * NAME:        query_program()
 * DESCRIPTION: return the program with the specified OID
 */
mixed *query_program(int oid)
{
    mixed *entry;

    ASSERT_ACCESS(previous_object() == objectd_);
    entry = pdb_entries_[oid];
    if (!entry) {
        error("No such program");
    }
    return entry;
}

/*
 * NAME:        get_program_dir()
 * DESCRIPTION: return all programs within a parent directory
 */
mapping get_program_dir(string path)
{
    int       i, size;
    string   *paths;
    mapping   names, directories, programs;

    ASSERT_ACCESS(previous_object() == objectd_);

    names = pdb_paths_[path + "/" .. path + "0"] - ({ path + "0" }); 
    paths = map_indices(names);
    directories = ([ ]);
    programs = ([ ]);
    size = sizeof(paths);
    for (i = 0; i < size; ++i) {
        string base;

        base = paths[i][strlen(path) + 1 ..];
        if (sscanf(base, "%s/", base) == 1) {
            directories[base] = TRUE;
        } else {
            programs[base] = names[paths[i]];
        }
    }

    paths = map_indices(directories);
    size = sizeof(paths);
    for (i = 0; i < size; ++i) {
        programs[paths[i]] = programs[paths[i]] ? ({ -2 }) + programs[paths[i]]
            : ({ -2 });
    }
    return programs;
}

/*
 * NAME:        add_data()
 * DESCRIPTION: register an LWO for management
 */
int add_data(object environment)
{
    int      oid, index;
    mapping  bucket;

    ASSERT_ACCESS(previous_object() == objectd_);
    oid = OID_LIGHTWEIGHT | (uid_ << OID_OWNER_OFFSET)
        | (next_index_++ << OID_INDEX_OFFSET);
    index = (oid & OID_INDEX_MASK) >> OID_INDEX_OFFSET;
    bucket = lightweight_oids_[(index - 1) / DATA_BUCKET_SIZE];
    if (!bucket) {
        bucket = lightweight_oids_[(index - 1) / DATA_BUCKET_SIZE] = ([ ]);
    }
    bucket[oid] = environment;
    return oid;
}

/*
 * NAME:        move_data()
 * DESCRIPTION: move a managed LWO to another environment
 */
void move_data(int oid, object environment)
{
    int      index;
    mapping  bucket;

    ASSERT_ACCESS(previous_object() == objectd_);
    index = (oid & OID_INDEX_MASK) >> OID_INDEX_OFFSET;
    bucket = lightweight_oids_[(index - 1) / DATA_BUCKET_SIZE];
    bucket[oid] = environment;
    if (!environment && !map_sizeof(bucket)) {
        lightweight_oids_[(index - 1) / DATA_BUCKET_SIZE] = nil;
    }
}

/*
 * NAME:        data_callout()
 * DESCRIPTION: schedule a call-out for a managed LWO
 */
int data_callout(int oid, string function, mixed delay, mixed *arguments)
{
    ASSERT_ACCESS(previous_object() == objectd_);
    return call_out("call_data", delay, oid, function, arguments);
}

/*
 * NAME:        remove_data_callout()
 * DESCRIPTION: remove a call-out for a managed LWO
 */
mixed remove_data_callout(int oid, int handle)
{
    int      i;
    mixed  **callouts;

    ASSERT_ACCESS(previous_object() == objectd_);
    callouts = status(this_object())[O_CALLOUTS];
    for (i = sizeof(callouts) - 1; i >= 0; --i) {
        if (callouts[i][CO_HANDLE] == handle) {
            /*
             * Found the call-out. Remove it only if it belongs to the managed
             * LWO.
             */
            return (callouts[i][CO_FIRSTXARG] == oid)
                ? remove_call_out(handle) : -1;
        }
    }
    return -1;
}

/*
 * NAME:        query_data_callouts()
 * DESCRIPTION: return the call-outs of a managed LWO
 */
mixed *query_data_callouts(string owner, int oid)
{
    int      i, j, size, owned;
    mixed  **callouts;

    ASSERT_ACCESS(previous_object() == objectd_);

    /* filter call-outs by object number */
    callouts = status(this_object())[O_CALLOUTS];
    size = sizeof(callouts);
    owned = (owner && owner == query_owner());
    for (i = j = 0; i < size; ++i) {
        if (callouts[i][CO_FIRSTXARG] == oid) {
            mixed *callout;

            callout = ({ callouts[i][CO_HANDLE],
                         callouts[i][CO_FIRSTXARG + 1],
                         callouts[i][CO_DELAY] });
            if (owned) {
                /* include arguments */
                callout += callouts[i][CO_FIRSTXARG + 2];
            }
            callouts[j++] = callout;
        }
    }
    return callouts[.. j - 1];
}

/*
 * NAME:        call_data()
 * DESCRIPTION: dispatch a call-out to a managed LWO
 */
static void call_data(int oid, string function, mixed *arguments)
{
    int      index;
    mapping  bucket;
    object   environment, obj;

    index = (oid & OID_INDEX_MASK) >> OID_INDEX_OFFSET;
    bucket = lightweight_oids_[(index - 1) / DATA_BUCKET_SIZE];
    if (!bucket) {
        return;
    }
    environment = bucket[oid];
    if (!environment) {
        return;
    }
    obj = environment->_F_find(oid);
    if (obj) {
        obj->_F_call_data(function, arguments);
    }
}