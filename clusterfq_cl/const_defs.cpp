#include "const_defs.h"

namespace ClusterFQ {
	/* ---------- */
	/* PARAMETERS */
	/* ---------- */

	const char* PARAM_IDENTITY_ID = "identity_id";	/* INT */

	const char* PARAM_CONTACT_ID = "contact_id";		/* INT */

	const char* PARAM_NAME = "name";					/* STRING */

	const char* PARAM_ADDRESS = "address";			/* STRING */

	const char* PARAM_TIME_START = "time_start";		/* LONG */

	const char* PARAM_TIME_END = "time_end";			/* LONG */

	const char* PARAM_TYPE = "type";					/* STRING */
	const char* PARAM_TYPE_VALUE_TEXT = "text";
	const char* PARAM_TYPE_VALUE_FILE = "file";

	const char* PARAM_FILENAME = "filename";			/* STRING */

	const char* PARAM_POST = "post";					/* STRING */


	/* --------- */
	/* PACKETSET */
	/* --------- */

	const char* MODULE_PACKETSET_GET = "GET /packetset/";
	/* GET */
	const char* PACKETSET_GET_ACTION_POLL = "poll";


	/* -------- */
	/* IDENTITY */
	/* -------- */

	const char* MODULE_IDENTITY_GET = "GET /identity/";
	/* GET */
	const char* IDENTITY_GET_ACTION_LIST = "list";

	const char* IDENTITY_GET_ACTION_CONTACT_LIST = "contact_list";
	const char* IDENTITY_GET_ACTION_CONTACT_LIST_PARAM_IDENTITY_ID = PARAM_IDENTITY_ID;


	const char* MODULE_IDENTITY_POST = "POST /identity/";
	/* POST */
	const char* IDENTITY_POST_ACTION_LOAD_ALL = "load_all";

	const char* IDENTITY_POST_ACTION_CREATE = "create";
	const char* IDENTITY_POST_ACTION_CREATE_PARAM_NAME = PARAM_NAME;

	const char* IDENTITY_POST_ACTION_CONTACT_ADD = "contact_add";
	const char* IDENTITY_POST_ACTION_CONTACT_ADD_PARAM_NAME = PARAM_NAME;
	const char* IDENTITY_POST_ACTION_CONTACT_ADD_PARAM_ADDRESS = PARAM_ADDRESS;
	const char* IDENTITY_POST_ACTION_CONTACT_ADD_PARAM_PUBKEY = PARAM_POST;

	const char* IDENTITY_POST_ACTION_CONTACT_DELETE = "contact_delete";
	const char* IDENTITY_POST_ACTION_CONTACT_DELETE_PARAM_IDENTITY_ID = PARAM_IDENTITY_ID;
	const char* IDENTITY_POST_ACTION_CONTACT_DELETE_PARAM_CONTACT_ID = PARAM_CONTACT_ID;

	const char* IDENTITY_POST_ACTION_CONTACT_VERIFY = "contact_verify";
	const char* IDENTITY_POST_ACTION_CONTACT_VERIFY_PARAM_IDENTITY_ID = PARAM_IDENTITY_ID;
	const char* IDENTITY_POST_ACTION_CONTACT_VERIFY_PARAM_CONTACT_ID = PARAM_CONTACT_ID;

	const char* IDENTITY_POST_ACTION_SHARE = "share";
	const char* IDENTITY_POST_ACTION_SHARE_PARAM_IDENTITY_ID = PARAM_IDENTITY_ID;
	const char* IDENTITY_POST_ACTION_SHARE_PARAM_NAME = PARAM_NAME;

	const char* IDENTITY_POST_ACTION_MIGRATE_KEY = "migrate_key";
	const char* IDENTITY_POST_ACTION_MIGRATE_KEY_PARAM_IDENTITY_ID = PARAM_IDENTITY_ID;

	const char* IDENTITY_POST_ACTION_REMOVE_OBSOLETE_KEYS = "remove_obsolete_keys";
	const char* IDENTITY_POST_ACTION_REMOVE_OBSOLETE_KEYS_PARAM_IDENTITY_ID = PARAM_IDENTITY_ID;

	const char* IDENTITY_POST_ACTION_DELETE = "delete";
	const char* IDENTITY_POST_ACTION_DELETE_PARAM_IDENTITY_ID = PARAM_IDENTITY_ID;


	/* ------- */
	/* CONTACT */
	/* ------- */

	const char* MODULE_CONTACT_GET = "GET /contact/";
	/* GET */
	const char* CONTACT_GET_ACTION_CHAT = "chat";
	const char* CONTACT_GET_ACTION_CHAT_PARAM_IDENTITY_ID = PARAM_IDENTITY_ID;
	const char* CONTACT_GET_ACTION_CHAT_PARAM_CONTACT_ID = PARAM_CONTACT_ID;
	const char* CONTACT_GET_ACTION_CHAT_PARAM_TIME_START = PARAM_TIME_START;
	const char* CONTACT_GET_ACTION_CHAT_PARAM_TIME_END = PARAM_TIME_END;


	/* ------- */
	/* MESSAGE */
	/* ------- */

	const char* MODULE_MESSAGE_POST = "POST /message/";
	/* POST */
	const char* MESSAGE_POST_ACTION_SEND = "send";
	const char* MESSAGE_POST_ACTION_SEND_PARAM_IDENTITY_ID = PARAM_IDENTITY_ID;
	const char* MESSAGE_POST_ACTION_SEND_PARAM_CONTACT_ID = PARAM_CONTACT_ID;
	const char* MESSAGE_POST_ACTION_SEND_PARAM_TYPE = PARAM_TYPE;
	const char* MESSAGE_POST_ACTION_SEND_PARAM_FILENAME = PARAM_FILENAME; /* IF PARAM_TYPE == FILE */
	const char* MESSAGE_POST_ACTION_SEND_PARAM_MESSAGE = PARAM_POST;

}