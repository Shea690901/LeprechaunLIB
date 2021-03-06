# include <limits.h>
# include <game/action.h>
# include <game/command.h>
# include <game/guild.h>
# include <game/race.h>
# include <game/word.h>
# include <game/thing.h>
# include <system/assert.h>

# define BAG     "/usr/Game/obj/bag"
# define CRYPT   "/usr/Game/room/crypt"
# define DWARF   "/usr/Game/obj/dwarf"
# define ELF     "/usr/Game/obj/elf"
# define SHIELD  "/usr/Game/data/shield"
# define SWORD   "/usr/Game/data/sword"
# define TEMPLE  "/usr/Game/room/temple"

object wordd_;
object commandd_;
object temple_;

static void create()
{
    object LIB_ROOM crypt;

    compile_object(DROP_ACTION);
    compile_object(GIVE_ACTION);
    compile_object(GO_ACTION);
    compile_object(INVENTORY_ACTION);
    compile_object(LOOK_ACTION);
    compile_object(LOOK_TO_ACTION);
    compile_object(PUT_IN_ACTION);
    compile_object(RELEASE_ACTION);
    compile_object(REMOVE_ACTION);
    compile_object(SAY_ACTION);
    compile_object(SCORE_ACTION);
    compile_object(TAKE_ACTION);
    compile_object(TAKE_FROM_ACTION);
    compile_object(WEAR_ACTION);
    compile_object(WIELD_ACTION);

    wordd_ = compile_object(WORDD);
    commandd_ = compile_object(COMMANDD);

    compile_object(ELF_RACE);
    compile_object(DWARF_RACE);
    compile_object(GOBLIN_RACE);
    compile_object(HUMAN_RACE);
    compile_object(LEPRECHAUN_RACE);

    compile_object(BARD_GUILD);
    compile_object(KNIGHT_GUILD);
    compile_object(MONK_GUILD);
    compile_object(PRIEST_GUILD);
    compile_object(RANGER_GUILD);
    compile_object(THIEF_GUILD);
    compile_object(WARRIOR_GUILD);
    compile_object(WIZARD_GUILD);

    compile_object(CREATURE);

    temple_ = compile_object(TEMPLE);
    crypt = compile_object(CRYPT); 
    compile_object(DWARF);
    compile_object(ELF);
    compile_object(CORPSE);
    compile_object(BAG);
    compile_object(SWORD);
    compile_object(SHIELD);
    compile_object(COIN);

    clone_object(BAG)->move(temple_);
    clone_object(DWARF)->move(crypt);
    new_object(SWORD)->move(crypt);
    new_object(SHIELD)->move(crypt);
    new_object(COIN, "silver", 1 + random(20))->move(crypt);
}

static string pick_element(string *array)
{
    return array[random(sizeof(array))];
}

static string pick_gender(string *options)
{
    string *genders;

    genders = ({ "female", "male" });
    options &= genders;
    return pick_element(sizeof(options) ? options : genders);
}

static object LIB_RACE pick_race(string *options)
{
    string *races, race;

    races = ({ "dwarf", "elf", "goblin", "human", "leprechaun" });
    options &= races;
    race = pick_element(sizeof(options) ? options : races);
    return find_object(RACE_DIR + "/" + race);
}

static object LIB_GUILD pick_guild(string *options)
{
    string *guilds, guild;

    guilds = ({ "bard", "knight", "monk", "priest", "ranger", "thief",
                "warrior", "wizard" });
    options &= guilds;
    guild = pick_element(sizeof(options) ? options : guilds);
    return find_object(GUILD_DIR + "/" + guild);
}

object LIB_CREATURE make_creature(varargs string str)
{
    string  *options, gender;

    object LIB_RACE      race;
    object LIB_GUILD     guild;
    object LIB_CREATURE  creature;

    options = str ? explode(str, " ") - ({ "" }) : ({ });

    creature = clone_object(CREATURE);
    creature->set_gender(pick_gender(options));
    creature->set_race(pick_race(options));
    creature->set_guild(pick_guild(options));
    
    creature->move(temple_);
    return creature;
}
