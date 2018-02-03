//
// Created by MightyPork on 2017/11/25.
//

#include "comm/messages.h"
#include "unit_base.h"
#include "unit_test.h"

/** Private data structure */
struct priv {
    uint32_t unused;
};

// ------------------------------------------------------------------------

/** Load from a binary buffer stored in Flash */
static void Tst_loadBinary(Unit *unit, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    //
}

/** Write to a binary buffer for storing in Flash */
static void Tst_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    //
}

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
static error_t Tst_loadIni(Unit *unit, const char *key, const char *value)
{
    return E_BAD_KEY;
}

/** Generate INI file section for the unit */
static void Tst_writeIni(Unit *unit, IniWriter *iw)
{
    struct priv *priv = unit->data;

    //
}

// ------------------------------------------------------------------------

/** Allocate data structure and set defaults */
static error_t Tst_preInit(Unit *unit)
{
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv));
    if (priv == NULL) return E_OUT_OF_MEM;

    //

    return E_SUCCESS;
}

/** Finalize unit set-up */
static error_t Tst_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    //

    return E_SUCCESS;
}

/** Tear down the unit */
static void Tst_deInit(Unit *unit)
{
    struct priv *priv = unit->data;

    //

    // Free memory
    free_ck(unit->data);
}

// ------------------------------------------------------------------------

enum PinCmd_ {
    CMD_PING = 0,
    CMD_ECHO = 1,
    CMD_BULKREAD = 2,
    CMD_BULKWRITE = 3,
};

// this is a very long text for testing bulk read
static const char *longtext = "The history of all hitherto existing societies is the history of class struggles.\n\nFreeman and slave, patrician and plebeian, lord and serf, guild-master and journeyman, in a word, oppressor and oppressed, stood in constant opposition to one another, carried on an uninterrupted, now hidden, now open fight, a fight that each time ended, either in a revolutionary re-constitution of society at large, or in the common ruin of the contending classes.\n\nIn the earlier epochs of history, we find almost everywhere a complicated arrangement of society into various orders, a manifold gradation of social rank. In ancient Rome we have patricians, knights, plebeians, slaves; in the Middle Ages, feudal lords, vassals, guild-masters, journeymen, apprentices, serfs; in almost all of these classes, again, subordinate gradations.\n\nThe modern bourgeois society that has sprouted from the ruins of feudal society has not done away with class antagonisms. It has but established new classes, new conditions of oppression, new forms of struggle in place of the old ones. Our epoch, the epoch of the bourgeoisie, possesses, however, this distinctive feature: it has simplified the class antagonisms. Society as a whole is more and more splitting up into two great hostile camps, into two great classes, directly facing each other: Bourgeoisie and Proletariat.\n\nFrom the serfs of the Middle Ages sprang the chartered burghers of the earliest towns. From these burgesses the first elements of the bourgeoisie were developed.\n\nThe discovery of America, the rounding of the Cape, opened up fresh ground for the rising bourgeoisie. The East-Indian and Chinese markets, the colonisation of America, trade with the colonies, the increase in the means of exchange and in commodities generally, gave to commerce, to navigation, to industry, an impulse never before known, and thereby, to the revolutionary element in the tottering feudal society, a rapid development.\n\nThe feudal system of industry, under which industrial production was monopolised by closed guilds, now no longer sufficed for the growing wants of the new markets. The manufacturing system took its place. The guild-masters were pushed on one side by the manufacturing middle class; division of labour between the different corporate guilds vanished in the face of division of labour in each single workshop.\n\nMeantime the markets kept ever growing, the demand ever rising. Even manufacture no longer sufficed. Thereupon, steam and machinery revolutionised industrial production. The place of manufacture was taken by the giant, Modern Industry, the place of the industrial middle class, by industrial millionaires, the leaders of whole industrial armies, the modern bourgeois.\n\nModern industry has established the world-market, for which the discovery of America paved the way. This market has given an immense development to commerce, to navigation, to communication by land. This development has, in its time, reacted on the extension of industry; and in proportion as industry, commerce, navigation, railways extended, in the same proportion the bourgeoisie developed, increased its capital, and pushed into the background every class handed down from the Middle Ages.\n\nWe see, therefore, how the modern bourgeoisie is itself the product of a long course of development, of a series of revolutions in the modes of production and of exchange.\n\nEach step in the development of the bourgeoisie was accompanied by a corresponding political advance of that class. An oppressed class under the sway of the feudal nobility, an armed and self-governing association in the mediaeval commune; here independent urban republic (as in Italy and Germany), there taxable \"third estate\" of the monarchy (as in France), afterwards, in the period of manufacture proper, serving either the semi-feudal or the absolute monarchy as a counterpoise against the nobility, and, in fact, corner-stone of the great monarchies in general, the bourgeoisie has at last, since the establishment of Modern Industry and of the world-market, conquered for itself, in the modern representative State, exclusive political sway. The executive of the modern State is but a committee for managing the common affairs of the whole bourgeoisie.";

static void br_longtext(struct bulk_read *bulk, uint32_t chunk, uint8_t *buffer)
{
    // clean-up request
    if (buffer == NULL) {
        free_ck(bulk);
        return;
    }

    memcpy(buffer, longtext+bulk->offset, chunk);
}

static void bw_dump(struct bulk_write *bulk, const uint8_t *chunk, uint32_t len)
{
    // clean-up request
    if (chunk == NULL) {
        free_ck(bulk);
        return;
    }

    dbg("\r\nBulk write at %d, len %d", (int)bulk->offset, (int)len);
    PUTSN((const char *) chunk, (uint16_t) len);
    PUTS("\r\n");
}

/** Handle a request message */
static error_t Tst_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    switch (command) {
        case CMD_PING:
            com_respond_ok(frame_id);
            return E_SUCCESS;

        case CMD_ECHO:;
            uint32_t len;
            const uint8_t *data = pp_tail(pp, &len);
            com_respond_buf(frame_id, MSG_SUCCESS, data, len);
            return E_SUCCESS;

        case CMD_BULKREAD:;
            BulkRead *br = malloc_ck(sizeof(struct bulk_read));
            assert_param(br);

            br->len = (uint32_t) strlen(longtext);
            br->frame_id = frame_id;
            br->read = br_longtext;

            bulkread_start(comm, br);
            return E_SUCCESS;

        case CMD_BULKWRITE:;
            BulkWrite *bw = malloc_ck(sizeof(struct bulk_write));
            assert_param(bw);

            bw->len = 10240;
            bw->frame_id = frame_id;
            bw->write = bw_dump;

            bulkwrite_start(comm, bw);
            return E_SUCCESS;

        default:
            return E_UNKNOWN_COMMAND;
    }
}

// ------------------------------------------------------------------------

/** Unit template */
const UnitDriver UNIT_TEST = {
    .name = "TEST",
    .description = "Test unit",
    // Settings
    .preInit = Tst_preInit,
    .cfgLoadBinary = Tst_loadBinary,
    .cfgWriteBinary = Tst_writeBinary,
    .cfgLoadIni = Tst_loadIni,
    .cfgWriteIni = Tst_writeIni,
    // Init
    .init = Tst_init,
    .deInit = Tst_deInit,
    // Function
    .handleRequest = Tst_handleRequest,
};
