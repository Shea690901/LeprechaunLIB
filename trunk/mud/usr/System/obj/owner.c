# include <status.h>
# include <system/assert.h>
# include <system/object.h>

object   objectd;    /* object manager */
int      uid;        /* user ID of owner */
mapping  programs;   /* ([ int oid: mixed *progent ]) */
int      nextindex;  /* index for the next managed LWO */
mapping  envs;       /* ([ int oid: object env ]) */

/*
 * NAME:        create()
 * DESCRIPTION: initialize owner object
 */
static void create(int clone)
{
    if (clone) {
        objectd = find_object(OBJECTD);
        uid = objectd->query_uid(query_owner());
        programs = ([ ]);
	nextindex = -2;
	envs = ([ ]);
    }
}

/*
 * NAME:        compile()
 * DESCRIPTION: the given object has just been compiled
 */
void compile(mixed obj, string *inherited)
{
    int oid, i, size;

    ASSERT_ACCESS(previous_object() == objectd);
    oid = objectd->join_oid(uid, status(obj)[O_INDEX]);
    size = sizeof(inherited);
    for (i = 0; i < size; ++i) {
    }
}

/*
 * NAME:        remove_program()
 * DESCRIPTION: the last reference to the given program has been removed
 */
void remove_program(int uid, int index)
{
    int oid;

    ASSERT_ACCESS(previous_object() == objectd);
    oid = objectd->join_oid(uid, index);
}

/*
 * NAME:        add_data()
 * DESCRIPTION: register an LWO for management
 */
int add_data(object env)
{
    int oid;

    ASSERT_ACCESS(previous_object() == objectd);
    DEBUG_ASSERT(uid);
    DEBUG_ASSERT(env);
    oid = objectd->join_oid(uid, nextindex--);
    envs[oid] = env;
    return oid;
}

/*
 * NAME:        find_data()
 * DESCRIPTION: find a managed LWO by number
 */
object find_data(int oid)
{
    object env;

    ASSERT_ACCESS(previous_object() == objectd);
    DEBUG_ASSERT(oid <= -2);
    env = envs[oid];
    return env ? env->_F_find(oid) : nil;
}

/*
 * NAME:        move_data()
 * DESCRIPTION: move a managed LWO to another environment
 */
void move_data(int oid, object env)
{
    ASSERT_ACCESS(previous_object() == objectd);
    DEBUG_ASSERT(oid <= -2 && envs[oid]);
    envs[oid] = env;
}

/*
 * NAME:        data_callout()
 * DESCRIPTION: schedule a call-out for a managed LWO
 */
int data_callout(int oid, string func, mixed delay, mixed *args)
{
    ASSERT_ACCESS(previous_object() == objectd);
    DEBUG_ASSERT(oid);
    DEBUG_ASSERT(func);
    DEBUG_ASSERT(args);
    return call_out("call_data", delay, oid, func, args);
}

/*
 * NAME:        remove_data_callout()
 * DESCRIPTION: remove a call-out for a managed LWO
 */
mixed remove_data_callout(int oid, int handle)
{
    int      i, size;
    mixed  **callouts;

    ASSERT_ACCESS(previous_object() == objectd);
    DEBUG_ASSERT(oid);

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

    ASSERT_ACCESS(previous_object() == objectd);

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
static void call_data(int oid, string func, mixed *args)
{
    object env;

    env = envs[oid];
    if (env) {
        object obj;

        obj = env->_F_find(oid);
        if (obj) {
            obj->_F_call_data(func, args);
        }
    }
}
