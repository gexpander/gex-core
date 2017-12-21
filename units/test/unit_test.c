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
static bool Tst_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    //

    return suc;
}

/** Generate INI file section for the unit */
static void Tst_writeIni(Unit *unit, IniWriter *iw)
{
    struct priv *priv = unit->data;

    //
}

// ------------------------------------------------------------------------

/** Allocate data structure and set defaults */
static bool Tst_preInit(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv), &suc);
    CHECK_SUC();

    //

    return true;
}

/** Finalize unit set-up */
static bool Tst_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    //

    return true;
}

/** Tear down the unit */
static void Tst_deInit(Unit *unit)
{
    struct priv *priv = unit->data;

    //

    // Free memory
    free(unit->data);
    unit->data = NULL;
}

// ------------------------------------------------------------------------

enum PinCmd_ {
    CMD_PING = 0,
    CMD_ECHO = 1,
    CMD_BULKREAD = 2,
};

static void job_echo(Job *job)
{
    tf_respond_buf(MSG_SUCCESS, job->frame_id, job->buf, job->len);
    free(job->buf);
}

const char *longtext = "The history of all hitherto existing societies is the history of class struggles.\n\nFreeman and slave, patrician and plebeian, lord and serf, guild-master and journeyman, in a word, oppressor and oppressed, stood in constant opposition to one another, carried on an uninterrupted, now hidden, now open fight, a fight that each time ended, either in a revolutionary re-constitution of society at large, or in the common ruin of the contending classes.\n\nIn the earlier epochs of history, we find almost everywhere a complicated arrangement of society into various orders, a manifold gradation of social rank. In ancient Rome we have patricians, knights, plebeians, slaves; in the Middle Ages, feudal lords, vassals, guild-masters, journeymen, apprentices, serfs; in almost all of these classes, again, subordinate gradations.\n\nThe modern bourgeois society that has sprouted from the ruins of feudal society has not done away with class antagonisms. It has but established new classes, new conditions of oppression, new forms of struggle in place of the old ones. Our epoch, the epoch of the bourgeoisie, possesses, however, this distinctive feature: it has simplified the class antagonisms. Society as a whole is more and more splitting up into two great hostile camps, into two great classes, directly facing each other: Bourgeoisie and Proletariat.\n\nFrom the serfs of the Middle Ages sprang the chartered burghers of the earliest towns. From these burgesses the first elements of the bourgeoisie were developed.\n\nThe discovery of America, the rounding of the Cape, opened up fresh ground for the rising bourgeoisie. The East-Indian and Chinese markets, the colonisation of America, trade with the colonies, the increase in the means of exchange and in commodities generally, gave to commerce, to navigation, to industry, an impulse never before known, and thereby, to the revolutionary element in the tottering feudal society, a rapid development.\n\nThe feudal system of industry, under which industrial production was monopolised by closed guilds, now no longer sufficed for the growing wants of the new markets. The manufacturing system took its place. The guild-masters were pushed on one side by the manufacturing middle class; division of labour between the different corporate guilds vanished in the face of division of labour in each single workshop.\n\nMeantime the markets kept ever growing, the demand ever rising. Even manufacture no longer sufficed. Thereupon, steam and machinery revolutionised industrial production. The place of manufacture was taken by the giant, Modern Industry, the place of the industrial middle class, by industrial millionaires, the leaders of whole industrial armies, the modern bourgeois.\n\nModern industry has established the world-market, for which the discovery of America paved the way. This market has given an immense development to commerce, to navigation, to communication by land. This development has, in its time, reacted on the extension of industry; and in proportion as industry, commerce, navigation, railways extended, in the same proportion the bourgeoisie developed, increased its capital, and pushed into the background every class handed down from the Middle Ages.\n\nWe see, therefore, how the modern bourgeoisie is itself the product of a long course of development, of a series of revolutions in the modes of production and of exchange.\n\nEach step in the development of the bourgeoisie was accompanied by a corresponding political advance of that class. An oppressed class under the sway of the feudal nobility, an armed and self-governing association in the mediaeval commune; here independent urban republic (as in Italy and Germany), there taxable \"third estate\" of the monarchy (as in France), afterwards, in the period of manufacture proper, serving either the semi-feudal or the absolute monarchy as a counterpoise against the nobility, and, in fact, corner-stone of the great monarchies in general, the bourgeoisie has at last, since the establishment of Modern Industry and of the world-market, conquered for itself, in the modern representative State, exclusive political sway. The executive of the modern State is but a committee for managing the common affairs of the whole bourgeoisie.";

static void job_bulkread_chunk(Job *job)
{
    dbg("Tx a chunk");
    tf_respond_buf(MSG_BULK_DATA, job->frame_id, job->buf, job->len);
}

static void job_bulkread_close(Job *job)
{
    dbg("Bulk read close.");
    tf_respond_buf(MSG_BULK_END, job->frame_id, NULL, 0);
}

static void job_bulkread_offer(Job *job)
{
    uint8_t buf[10];
    PayloadBuilder pb = pb_start(buf, 10, NULL);
    pb_u32(&pb, job->d32);

    dbg("Offer bulk xfer of %d bytes", job->d32);

    tf_respond_buf(MSG_BULK_READ_OFFER, job->frame_id, buf, (uint32_t) pb_length(&pb));
}

static TF_Result bulkread_lst(TinyFrame *tf, TF_Msg *msg)
{
    if (msg->data == NULL) {
        dbg("Bulk rx lst cleanup\r\n");
        return TF_CLOSE;
    } // this is a final call before timeout, to clean up

    if (msg->type == MSG_BULK_READ_POLL) {
        uint32_t pos = (uint32_t) msg->userdata2;
        uint32_t total = (uint32_t) strlen(longtext); // normally we'd not calculate it here

        dbg("BR poll, at %d", pos);

        // Say we're done and close if it's over
        if (pos >= total) {
            Job job = {
                .cb = job_bulkread_close,
                .frame_id = msg->frame_id
            };
            scheduleJob(&job, TSK_SCHED_HIGH);
            return TF_CLOSE;
        }

        PayloadParser pp = pp_start(msg->data, msg->len, NULL);
        uint32_t chunk = pp_u32(&pp);
        chunk = MIN(chunk, total - pos);

        // this isn't at all resistant to error, we expect the communication to be lossless
        Job job = {
            .cb = job_bulkread_chunk,
            .frame_id = msg->frame_id,
            .buf = (uint8_t *) (longtext + pos),
            .len = chunk,
        };
        scheduleJob(&job, TSK_SCHED_HIGH);

        // advance the position pointer
        pos += chunk;
        msg->userdata2 = (void *) pos;
    }

    if (msg->type == MSG_BULK_ABORT) {
        return TF_CLOSE;
    }

    return TF_STAY;
}

/** Handle a request message */
static bool Tst_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    (void)pp;

    struct priv *priv = unit->data;

    switch (command) {
        case CMD_PING:
            sched_respond_suc(frame_id);
            break;

        case CMD_ECHO:;
            uint32_t len = (uint32_t) pp_length(pp);
            uint8_t *cpy = malloc(len);
            memcpy(cpy, pp_tail(pp, NULL), len);
            Job job = {
                .cb = job_echo,
                .frame_id = frame_id,
                .buf = cpy,
                .len = len,
            };
            dbg("Rx len %d, %.*s\r\n", len, len, cpy);
            scheduleJob(&job, TSK_SCHED_HIGH);
            break;

        case CMD_BULKREAD:;
            TF_Msg msg = {
                .frame_id = frame_id,
                .userdata = unit, // we'll keep a reference to the unit here
                .userdata2 = 0 // and current position here.
            };
            // in a real scenario, we'd put a malloc'd struct here.
            TF_AddIdListener(comm, &msg, bulkread_lst, 200);

            Job job2 = {
                .cb = job_bulkread_offer,
                .frame_id = frame_id,
                .d32 = (uint32_t) strlen(longtext),
            };
            scheduleJob(&job2, TSK_SCHED_HIGH);
            break;

        default:
            sched_respond_bad_cmd(frame_id);
            return false;
    }

    return true;
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
