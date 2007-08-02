# include <status.h>
# include <kernel/kernel.h>
# include <kernel/rsrc.h>
# include <kernel/tls.h>
# include <system/assert.h>
# include <system/object.h>
# include <system/system.h>

private inherit rsrc  API_RSRC;
private inherit tls   API_TLS;

object   driver_;      /* driver object */
object   initd_;       /* system initialization manager */
int      next_uid_;    /* next UID */
mapping  uids_;        /* ([ string owner: int uid ]) */
mapping  owner_objs_;  /* ([ int uid: object owner_obj ]) */

/*
 * NAME:        create()
 * DESCRIPTION: initialize object manager
 */
static void create()
{
    rsrc::create();
    tls::create();
    driver_ = find_object(DRIVER);
    initd_ = find_object(INITD);
    next_uid_ = 1;
    uids_ = ([ ]);
    owner_objs_ = ([ ]);
}

/*
 * NAME:        add_owner()
 * DESCRIPTION: register a new or existing owner
 */
private int add_owner(string owner)
{
    if (uids_[owner] == nil) {
        int uid;

	/* new owner: register and create owner object */
	uid = uids_[owner] = next_uid_++;
        owner_objs_[uid] = clone_object(OWNEROBJ, owner);
        driver_->message("OBJECTD: added owner " + owner + " with UID " + uid
                         + "\n");
    }
    return uids_[owner];
}

/*
 * NAME:        query_uid()
 * DESCRIPTION: return the user ID of an owner
 */
int query_uid(string owner)
{
    return uids_[owner] ? uids_[owner] : 0;
}

/*
 * NAME:        compile()
 * DESCRIPTION: the given object has just been compiled
 */
void compile(string owner, object obj, string *source, string inherited...)
{
    int uid;

    ASSERT_ACCESS(previous_object() == driver_ || previous_object() == initd_);
    uid = add_owner(owner);
    owner_objs_[uid]->compile(obj, source, inherited);
}

/*
 * NAME:        compile_lib()
 * DESCRIPTION: the given inheritable object has just been compiled
 */
void compile_lib(string owner, string path, string *source,
                 string inherited...)
{
    int uid;

    ASSERT_ACCESS(previous_object() == driver_ || previous_object() == initd_);
    uid = add_owner(owner);
    owner_objs_[uid]->compile(path, source, inherited);
}

/*
 * NAME:        clone()
 * DESCRIPTION: the given object has just been cloned
 */
void clone(string owner, object obj)
{
    int uid;

    ASSERT_ACCESS(previous_object() == driver_);
    if (sscanf(object_name(obj), OWNEROBJ + "#%*s")) {
        return;
    }
    uid = add_owner(owner);
    owner_objs_[uid]->clone(obj);
}

/*
 * NAME:        destruct()
 * DESCRIPTION: the given object is about to be destructed
 */
void destruct(string owner, object obj)
{
    int uid;

    ASSERT_ACCESS(previous_object() == driver_);
    uid = query_uid(owner);
    owner_objs_[uid]->destruct(obj);
}

/*
 * NAME:        remove_program()
 * DESCRIPTION: the last reference to the given program has been removed
 */
void remove_program(string owner, string path, int timestamp, int index)
{
    int uid;

    ASSERT_ACCESS(previous_object() == driver_);
    uid = uids_[owner];
    owner_objs_[uid]->remove_program(index);
}

/*
 * NAME:        include_file()
 * DESCRIPTION: returns an include path that depends on the compiled path
 */
mixed include_file(string compiled, string from, string path)
{
    ASSERT_ACCESS(previous_object() == driver_);
    if (from == "/include/std.h" && path == "/include/AUTO"
        && driver_->creator(compiled) != "System")
    {
        return ({ "inherit \"" + SYSTEM_AUTO + "\";\n" });
    } else {
        return path;
    }
}

/*
 * NAME:        forbid_call()
 * DESCRIPTION: returns true if a call is forbidden
 */
int forbid_call(string path)
{
    ASSERT_ACCESS(previous_object() == driver_);
    return sscanf(path, "%*s" + CLONABLE_SUBDIR)
        || sscanf(path, "%*s" + LIGHTWEIGHT_SUBDIR);
}

/*
 * NAME:        forbid_inherit()
 * DESCRIPTION: returns true if an inheritance is forbidden
 */
int forbid_inherit(string from, string path, int priv)
{
    string from_creator, path_creator;

    ASSERT_ACCESS(previous_object() == driver_);

    from_creator = driver_->creator(from);
    path_creator = driver_->creator(path);

    /* system objects cannot inherit user objects */
    if (from_creator == "System" && path_creator != "System") {
        return TRUE;
    }

    /*
     * user objects cannot inherit system objects, except for objects in
     * ~System/open
     */
    if (from_creator != "System" && path_creator == "System"
        && !sscanf(path, "/usr/System/open/%*s"))
    {
        return TRUE;
    }

    /* forbid private inheritance of undefined functions */
    if (priv) {
        mixed *status;
    
        status = status(path);
        if (status && status[O_UNDEFINED]) {
            driver_->message("Undefined functions cannot be privately "
                            + "inherited\n");
            return TRUE;
        }
    }

    return FALSE;
}

/*
 * NAME:        get_tlvar()
 * DESCRIPTION: proxies TLS reads for the system auto object
 */
mixed get_tlvar(int index)
{
    ASSERT_ACCESS(previous_program() == SYSTEM_AUTO);
    return tls::get_tlvar(index);
}

/*
 * NAME:        set_tlvar()
 * DESCRIPTION: proxies TLS writes for the system auto object
 */
void set_tlvar(int index, mixed value)
{
    ASSERT_ACCESS(previous_program() == SYSTEM_AUTO);
    tls::set_tlvar(index, value);
}

/*
 * NAME:        find_program()
 * DESCRIPTION: find a program by object ID
 */
mixed *find_program(int oid)
{
    int uid;

    ASSERT_ACCESS(previous_program() == OWNEROBJ);
    uid = (oid & OID_OWNER_MASK) >> OID_OWNER_OFFSET;
    return owner_objs_[uid]->find_program(oid);
}

static mapping program_dir_map(string *directories)
{
    int      i, size;
    mapping  map;

    map = ([ ]);
    size = sizeof(directories);
    for (i = 0; i < size; ++i) {
        map[directories[i]] = ({ -2 });
    }
    return map;
}

/*
 * NAME:        get_program_dir()
 * DESCRIPTION: return all programs within a parent directory
 */
mapping get_program_dir(string path)
{
    string  creator;
    mixed   uid;

    ASSERT_ACCESS(previous_program() == API_OBJECT);
    if (path == "/") {
        return program_dir_map(({ "kernel", USR_DIR[1 ..] }));
    } else if (path == USR_DIR) {
        return program_dir_map(query_owners() - ({ nil }));
    }

    creator = driver_->creator(path + "/");
    uid = uids_[creator];
    if (uid == nil) {
        return ([ ]);
    }
    return owner_objs_[uid]->get_program_dir(path);
}

/*
 * NAME:        add_data()
 * DESCRIPTION: add a managed LWO
 */
int add_data(string owner, object env)
{
    int uid;

    ASSERT_ACCESS(previous_program() == SYSTEM_AUTO);
    uid = add_owner(owner);
    return owner_objs_[uid]->add_data(env);
}

/*
 * NAME:        find()
 * DESCRIPTION: find a persistent object or managed LWO by number
 */
object find(int oid)
{
    int     uid;

    ASSERT_ACCESS(previous_program() == SYSTEM_AUTO);
    uid = (oid & OID_OWNER_MASK) >> OID_OWNER_OFFSET;
    return owner_objs_[uid] ? owner_objs_[uid]->find(oid) : nil;
}

/*
 * NAME:        move_data()
 * DESCRIPTION: move or remove a managed LWO
 */
void move_data(int oid, object environment)
{
    int uid;

    ASSERT_ACCESS(previous_program() == SYSTEM_AUTO);
    uid = (oid & OID_OWNER_MASK) >> OID_OWNER_OFFSET;
    owner_objs_[uid]->move_data(oid, environment);
}

/*
 * NAME:        data_callout()
 * DESCRIPTION: make a call-out for a managed LWO
 */
int data_callout(int oid, string function, mixed delay, mixed *arguments)
{
    int uid;

    ASSERT_ACCESS(previous_program() == SYSTEM_AUTO);
    uid = (oid & OID_OWNER_MASK) >> OID_OWNER_OFFSET;
    return owner_objs_[uid]->data_callout(oid, function, delay, arguments);
}

/*
 * NAME:        remove_data_callout()
 * DESCRIPTION: remove a call-out for a managed LWO
 */
mixed remove_data_callout(int oid, int handle)
{
    int uid;

    ASSERT_ACCESS(previous_program() == SYSTEM_AUTO);
    uid = (oid & OID_OWNER_MASK) >> OID_OWNER_OFFSET;
    return owner_objs_[uid]->remove_data_callout(oid, handle);
}

/*
 * NAME:        query_data_callouts()
 * DESCRIPTION: return the call-outs for a managed LWO
 */
mixed *query_data_callouts(string owner, int oid)
{
    int uid;

    ASSERT_ACCESS(previous_program() == SYSTEM_AUTO);
    uid = (oid & OID_OWNER_MASK) >> OID_OWNER_OFFSET;
    return owner_objs_[uid]->query_data_callouts(owner, oid);
}
