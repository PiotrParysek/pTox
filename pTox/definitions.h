#ifndef DEFINITIONS_H
#define DEFINITIONS_H

/* Connection delay in seconds */
#define CONNECTDELAY 3

/* Ringing delay in seconds */
#define RINGINGDELAY 16

/* Maximum number of simultaneous calls */
#define MAXCALLS 3

/* Audio settings definition */
#define AUDIOCHANNELS     1
#define AUDIOBITRATE      32
#define AUDIOFRAME        20
#define AUDIOSAMPLERATE   24000

#define LEN(x) (sizeof (x) / sizeof *(x))

/**
 * @brief The node struct of Tox Nodes
 */
struct node {
    char const  *addr4;
    uint16_t    port;
    char const  *key;
};
/**
 * Example array of Tox nodes
 */
static struct node nodes[] =
{
{ //US&A
    .addr4  = "node.tox.biribiri.org",
    .port   = 33445,
    .key    = "F404ABAA1C99A9D37D61AB54898F56793E1DEF8BD46B1038B9D822E8460FAB67"
},
{ //DEU
    .addr4  = "130.133.110.14",
    .port   = 33445,
    .key    = "461FA3776EF0FA655F1A05477DF1B3B614F7D6B124F7DB1DD4FE3C08B03B640F"
},
{ //RUS
    .addr4  = "85.172.30.117",
    .port   = 33445,
    .key    = "8E7D0B859922EF569298B4D261A8CCB5FEA14FB91ED412A7603A585A25698832"
},
{ //UKR
    .addr4  = "185.25.116.107",
    .port   = 33445,
    .key    = "DA4E4ED4B697F2E9B000EEFE3A34B554ACD3F45F5C96EAEA2516DD7FF9AF7B43"
},
{ //POL
    .addr4  = "77.55.211.53",
    .port   = 53,
    .key    = "B9D109CC820C69A5D97A4A1A15708107C6BA85C13BC6188CC809D374AFF18E63"
},
{ //SVN
    .addr4  = "194.249.212.109",
    .port   = 33445,
    .key    = "3CEE1F054081E7A011234883BC4FC39F661A55B73637A5AC293DDF1251D9432B"
}
//{
//    .addr4  = "",
//    .port   = ,
//    .key    = ""
//}
};

#endif // DEFINITIONS_H
