#ifndef __GMAIL_CONSTANTS
#define __GMAIL_CONSTANTS

#define D_VERSION	"v"
#define D_QUOTA	"qu"
#define D_DEFAULTSEARCH_SUMMARY	"ds"
#define D_THREADLIST_SUMMARY	"ts"
#define D_THREADLIST_END	"te"
#define D_THREAD	"t"
#define D_CONV_SUMMARY	"cs"
#define D_CONV_END	"ce"
#define D_MSGINFO	"mi"
#define D_MSGBODY	"mb"
#define D_MSGATT	"ma"
#define D_COMPOSE	"c"
#define D_CONTACT	"co"
#define D_CATEGORIES	"ct"
#define D_CATEGORIES_COUNT_ALL	"cta"
#define D_ACTION_RESULT	"ar"
#define D_SENDMAIL_RESULT	"sr"
#define D_PREFERENCES	"p"
#define D_PREFERENCES_PANEL	"pp"
#define D_FILTERS	"fi"
#define D_GAIA_NAME	"gn"
#define D_INVITE_STATUS	"i"
#define D_END_PAGE	"e"
#define D_LOADING	"l"
#define D_LOADED_SUCCESS	"ld"
#define D_LOADED_ERROR	"le"
#define D_QUICKLOADED	"ql"
#define D_COMPOSE_FROM_ADDRESS	"cfs"
#define D_FOOTER_TIP	"ft"
#define D_WEB_CLIP	"ad"
#define QU_SPACEUSED	0
#define QU_QUOTA	1
#define QU_PERCENT	2
#define QU_COLOR	3
#define TS_START	0
#define TS_NUM	1
#define TS_TOTAL	2
#define TS_ESTIMATES	3
#define TS_TITLE	4
#define TS_TIMESTAMP	5
#define TS_TOTAL_MSGS	6
#define T_THREADID	0
#define T_UNREAD	1
#define T_STAR	2
#define T_DATE_HTML	3
#define T_AUTHORS_HTML	4
#define T_FLAGS	5
#define T_SUBJECT_HTML	6
#define T_SNIPPET_HTML	7
#define T_CATEGORIES	8
#define T_ATTACH_HTML	9
#define T_MATCHING_MSGID	10
#define T_EXTRA_SNIPPET	11
#define CS_THREADID	0
#define CS_SUBJECT	1
#define CS_TITLE_HTML	2
#define CS_SUMMARY_HTML	3
#define CS_CATEGORIES	4
#define CS_PREVNEXTTHREADIDS	5
#define CS_THREAD_UPDATED	6
#define CS_NUM_MSGS	7
#define CS_ADKEY	8
#define CS_MATCHING_MSGID	9
#define MI_FLAGS	0
#define MI_NUM	1
#define MI_MSGID	2
#define MI_STAR	3
#define MI_REFMSG	4
#define MI_AUTHORNAME	5
#define MI_AUTHOREMAIL	6
#define MI_MINIHDRHTML	7
#define MI_DATEHTML	8
#define MI_TO	9
#define MI_CC	10
#define MI_BCC	11
#define MI_REPLYTO	12
#define MI_DATE	13
#define MI_SUBJECT	14
#define MI_SNIPPETHTML	15
#define MI_ATTACHINFO	16
#define MI_KNOWNAUTHOR	17
#define MI_PHISHWARNING	18
#define A_ID	0
#define A_FILENAME	1
#define A_MIMETYPE	2
#define A_FILESIZE	3
#define CT_NAME	0
#define CT_COUNT	1
#define AR_SUCCESS	0
#define AR_MSG	1
#define SM_COMPOSEID	0
#define SM_SUCCESS	1
#define SM_MSG	2
#define SM_NEWTHREADID	3
#define CMD_SEARCH	"SEARCH"
#define ACTION_TOKEN_COOKIE	"GMAIL_AT"
#define U_VIEW	"view"
#define U_PAGE_VIEW	"page"
#define U_THREADLIST_VIEW	"tl"
#define U_CONVERSATION_VIEW	"cv"
#define U_COMPOSE_VIEW	"cm"
#define U_PRINT_VIEW	"pt"
#define U_PREFERENCES_VIEW	"pr"
#define U_JSREPORT_VIEW	"jr"
#define U_UPDATE_VIEW	"up"
#define U_SENDMAIL_VIEW	"sm"
#define U_AD_VIEW	"ad"
#define U_REPORT_BAD_RELATED_INFO_VIEW	"rbri"
#define U_ADDRESS_VIEW	"address"
#define U_ADDRESS_IMPORT_VIEW	"ai"
#define U_SPELLCHECK_VIEW	"sc"
#define U_INVITE_VIEW	"invite"
#define U_ORIGINAL_MESSAGE_VIEW	"om"
#define U_ATTACHMENT_VIEW	"att"
#define U_DEBUG_ADS_RESPONSE_VIEW	"da"
#define U_SEARCH	"search"
#define U_INBOX_SEARCH	"inbox"
#define U_STARRED_SEARCH	"starred"
#define U_ALL_SEARCH	"all"
#define U_DRAFTS_SEARCH	"drafts"
#define U_SENT_SEARCH	"sent"
#define U_SPAM_SEARCH	"spam"
#define U_TRASH_SEARCH	"trash"
#define U_QUERY_SEARCH	"query"
#define U_ADVANCED_SEARCH	"adv"
#define U_CREATEFILTER_SEARCH	"cf"
#define U_CATEGORY_SEARCH	"cat"
#define U_AS_FROM	"as_from"
#define U_AS_TO	"as_to"
#define U_AS_SUBJECT	"as_subj"
#define U_AS_SUBSET	"as_subset"
#define U_AS_HAS	"as_has"
#define U_AS_HASNOT	"as_hasnot"
#define U_AS_ATTACH	"as_attach"
#define U_AS_WITHIN	"as_within"
#define U_AS_DATE	"as_date"
#define U_AS_SUBSET_ALL	"all"
#define U_AS_SUBSET_INBOX	"inbox"
#define U_AS_SUBSET_STARRED	"starred"
#define U_AS_SUBSET_SENT	"sent"
#define U_AS_SUBSET_DRAFTS	"drafts"
#define U_AS_SUBSET_SPAM	"spam"
#define U_AS_SUBSET_TRASH	"trash"
#define U_AS_SUBSET_ALLSPAMTRASH	"ast"
#define U_AS_SUBSET_READ	"read"
#define U_AS_SUBSET_UNREAD	"unread"
#define U_AS_SUBSET_CATEGORY_PREFIX	"cat_"
#define U_THREAD	"th"
#define U_PREV_THREAD	"prev"
#define U_NEXT_THREAD	"next"
#define U_DRAFT_MSG	"draft"
#define U_START	"start"
#define U_ACTION	"act"
#define U_ACTION_TOKEN	"at"
#define U_INBOX_ACTION	"ib"
#define U_MARKREAD_ACTION	"rd"
#define U_MARKUNREAD_ACTION	"ur"
#define U_MARKSPAM_ACTION	"sp"
#define U_UNMARKSPAM_ACTION	"us"
#define U_MARKTRASH_ACTION	"tr"
#define U_ADDCATEGORY_ACTION	"ac_"
#define U_REMOVECATEGORY_ACTION	"rc_"
#define U_ADDSTAR_ACTION	"st"
#define U_REMOVESTAR_ACTION	"xst"
#define U_ADDSENDERTOCONTACTS_ACTION	"astc"
#define U_DELETEMESSAGE_ACTION	"dm"
#define U_DELETE_ACTION	"dl"
#define U_EMPTYSPAM_ACTION	"es_"
#define U_EMPTYTRASH_ACTION	"et_"
#define U_SAVEPREFS_ACTION	"prefs"
#define U_ADDRESS_ACTION	"a"
#define U_CREATECATEGORY_ACTION	"cc_"
#define U_DELETECATEGORY_ACTION	"dc_"
#define U_RENAMECATEGORY_ACTION	"nc_"
#define U_CREATEFILTER_ACTION	"cf"
#define U_REPLACEFILTER_ACTION	"rf"
#define U_DELETEFILTER_ACTION	"df_"
#define U_ACTION_THREAD	"t"
#define U_ACTION_MESSAGE	"m"
#define U_ACTION_PREF_PREFIX	"p_"
#define U_REFERENCED_MSG	"rm"
#define U_COMPOSEID	"cmid"
#define U_COMPOSE_MODE	"cmode"
#define U_COMPOSE_SUBJECT	"su"
#define U_COMPOSE_TO	"to"
#define U_COMPOSE_CC	"cc"
#define U_COMPOSE_BCC	"bcc"
#define U_COMPOSE_BODY	"body"
#define U_PRINT_THREAD	"pth"
#define CONV_VIEW	"conv"
#define TLIST_VIEW	"tlist"
#define PREFS_VIEW	"prefs"
#define HIST_VIEW	"hist"
#define COMPOSE_VIEW	"comp"
#define HIDDEN_ACTION	0
#define USER_ACTION	1
#define BACKSPACE_ACTION	2

#endif
