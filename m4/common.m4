m4_dnl Radical change to quoting so backquote works:
m4_changequote(«,»)m4_dnl
m4_define(COMMENT,)m4_dnl
m4_define(m4_getenv,«m4_esyscmd(printf -- $«$1»)»)m4_dnl
