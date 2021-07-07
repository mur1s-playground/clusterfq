#pragma once

namespace ClusterFQ {

	/* ---------- */
	/* PARAMETERS */
	/* ---------- */

	extern const char* PARAM_IDENTITY_ID;	/* INT */

	extern const char* PARAM_CONTACT_ID;	/* INT */

	extern const char* PARAM_NAME;			/* STRING */

	extern const char* PARAM_ADDRESS;		/* STRING */

	extern const char* PARAM_TIME_START;	/* LONG */

	extern const char* PARAM_TIME_END;		/* LONG */

	extern const char* PARAM_TYPE;			/* STRING */
	extern const char* PARAM_TYPE_VALUE_TEXT;
	extern const char* PARAM_TYPE_VALUE_FILE;

	extern const char* PARAM_FILENAME;		/* STRING */

	extern const char* PARAM_POST;			/* STRING */


	/* --------- */
	/* PACKETSET */
	/* --------- */

	extern const char* MODULE_PACKETSET_GET;
	/* GET */
	extern const char* PACKETSET_GET_ACTION_POLL;


	/* -------- */
	/* IDENTITY */
	/* -------- */

	extern const char* MODULE_IDENTITY_GET;
	/* GET */
	extern const char* IDENTITY_GET_ACTION_LIST;

	extern const char* IDENTITY_GET_ACTION_CONTACT_LIST;
	extern const char* IDENTITY_GET_ACTION_CONTACT_LIST_PARAM_IDENTITY_ID;


	extern const char* MODULE_IDENTITY_POST;
	/* POST */
	extern const char* IDENTITY_POST_ACTION_LOAD_ALL;

	extern const char* IDENTITY_POST_ACTION_CREATE;
	extern const char* IDENTITY_POST_ACTION_CREATE_PARAM_NAME;

	extern const char* IDENTITY_POST_ACTION_CONTACT_ADD;
	extern const char* IDENTITY_POST_ACTION_CONTACT_ADD_PARAM_NAME;
	extern const char* IDENTITY_POST_ACTION_CONTACT_ADD_PARAM_ADDRESS;
	extern const char* IDENTITY_POST_ACTION_CONTACT_ADD_PARAM_PUBKEY;

	extern const char* IDENTITY_POST_ACTION_CONTACT_DELETE;
	extern const char* IDENTITY_POST_ACTION_CONTACT_DELETE_PARAM_IDENTITY_ID;
	extern const char* IDENTITY_POST_ACTION_CONTACT_DELETE_PARAM_CONTACT_ID;

	extern const char* IDENTITY_POST_ACTION_CONTACT_VERIFY;
	extern const char* IDENTITY_POST_ACTION_CONTACT_VERIFY_PARAM_IDENTITY_ID;
	extern const char* IDENTITY_POST_ACTION_CONTACT_VERIFY_PARAM_CONTACT_ID;

	extern const char* IDENTITY_POST_ACTION_SHARE;
	extern const char* IDENTITY_POST_ACTION_SHARE_PARAM_IDENTITY_ID;
	extern const char* IDENTITY_POST_ACTION_SHARE_PARAM_NAME;

	extern const char* IDENTITY_POST_ACTION_MIGRATE_KEY;
	extern const char* IDENTITY_POST_ACTION_MIGRATE_KEY_PARAM_IDENTITY_ID;

	extern const char* IDENTITY_POST_ACTION_REMOVE_OBSOLETE_KEYS;
	extern const char* IDENTITY_POST_ACTION_REMOVE_OBSOLETE_KEYS_PARAM_IDENTITY_ID;

	extern const char* IDENTITY_POST_ACTION_DELETE;
	extern const char* IDENTITY_POST_ACTION_DELETE_PARAM_IDENTITY_ID;


	/* ------- */
	/* CONTACT */
	/* ------- */

	extern const char* MODULE_CONTACT_GET;
	/* GET */
	extern const char* CONTACT_GET_ACTION_CHAT;
	extern const char* CONTACT_GET_ACTION_CHAT_PARAM_IDENTITY_ID;
	extern const char* CONTACT_GET_ACTION_CHAT_PARAM_CONTACT_ID;
	extern const char* CONTACT_GET_ACTION_CHAT_PARAM_TIME_START;
	extern const char* CONTACT_GET_ACTION_CHAT_PARAM_TIME_END;


	/* ------- */
	/* MESSAGE */
	/* ------- */

	extern const char* MODULE_MESSAGE_POST;
	/* POST */
	extern const char* MESSAGE_POST_ACTION_SEND;
	extern const char* MESSAGE_POST_ACTION_SEND_PARAM_IDENTITY_ID;
	extern const char* MESSAGE_POST_ACTION_SEND_PARAM_CONTACT_ID;
	extern const char* MESSAGE_POST_ACTION_SEND_PARAM_TYPE;
	extern const char* MESSAGE_POST_ACTION_SEND_PARAM_FILENAME; /* IF PARAM_TYPE == FILE */
	extern const char* MESSAGE_POST_ACTION_SEND_PARAM_MESSAGE;
}