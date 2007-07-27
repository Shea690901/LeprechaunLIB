# include <game/action.h>
# include <game/description.h>
# include <game/message.h>
# include <game/thing.h>
# include <system/assert.h>

inherit LIB_THING;
private inherit UTIL_DESCRIPTION;
private inherit UTIL_MESSAGE;

string race_;

static void create()
{
    ::create();
    add_event("observe");
    add_event("error");
}

static void set_race(string race)
{
    race_ = race;
    add_noun(race);
    set_look(indefinite_article(race) + " " + race);
}

int allow_subscribe(object obj, string name)
{
    return TRUE;
}

void observe(string mess)
{
    event("observe", mess);
}

void add_action(object LIB_ACTION action)
{
    ASSERT_ARG(action);
    call_out("act", 0, action);
}

static void act(object LIB_ACTION action)
{
    string error;

    error = catch(action->perform(this_object()));
    if (error) {
        event("error", error);
    }
}

static void drop_all()
{
    object LIB_ITEM  *inv;
    object LIB_ROOM   env;

    int i, size;

    env = environment(this_object());
    inv = inventory(this_object());
    size = sizeof(inv);
    for (i = 0; i < size; ++i) {
        move_object(inv[i], env);
    }
}

static void make_corpse()
{
    object LIB_ROOM env;

    env = environment(this_object());
    if (race_ && env) {
        move_object(new_object(CORPSE, race_), env);
    }
}

void die()
{
    drop_all();
    make_corpse();
    tell_audience(this_object(),
                  definite_description(this_object()) + " dies.");
    destruct_object(this_object());
}
