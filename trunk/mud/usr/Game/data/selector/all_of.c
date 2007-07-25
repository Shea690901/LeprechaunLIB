# include <game/selector.h>
# include <game/thing.h>

inherit LIB_SELECTOR;

string *words_;

static void create(string *words)
{
    words_ = words;
}

object LIB_THING *select(object LIB_THING *objs,
                         varargs object LIB_CREATURE actor)
{
    int i, j, size;
    object LIB_THING *res;

    size = sizeof(objs);
    res = allocate(size);
    for (i = j = 0; i < size; ++i) {
        if (objs[i]->identify(words_, actor)) {
            res[j++] = objs[i];
        }
    }
    return res[.. j - 1];
}