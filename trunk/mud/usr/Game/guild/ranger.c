# include <game/guild.h>

inherit LIB_GUILD;

static void create()
{
    ::create();
    set_name("Ranger");
    add_noun("ranger");

    set_trait_bonus("strength",       0.0);
    set_trait_bonus("constitution",   1.0);
    set_trait_bonus("dexterity",      1.0);
    set_trait_bonus("charisma",      -1.0);
    set_trait_bonus("intelligence",   0.0);
    set_trait_bonus("wisdom",        -1.0);
}
