# include <game/thing.h>

inherit LIB_CONTAINER;

static void create()
{
    ::create();
    add_noun("bag");
    set_look("a bag");
}
