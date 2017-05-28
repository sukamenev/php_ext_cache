#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "php_cache.h"

#include "callin.h"
#include "stdlib.h"

const zend_function_entry cache_functions[] = {
	PHP_FE(cach_set_dir, NULL)
	PHP_FE(cach_connect, NULL)
	PHP_FE(cach_quit, NULL)
	PHP_FE(cach_set, NULL)
	PHP_FE(cach_get,NULL)
	PHP_FE(cach_kill, NULL)
	PHP_FE(cach_kill_tree,NULL)
	PHP_FE(cach_order, NULL)
	PHP_FE(cach_order_rev, NULL)
	PHP_FE(cach_exec, NULL)
	PHP_FE(test, NULL)
	PHP_FE_END
};

zend_module_entry cache_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	PHP_CACHE_EXT_NAME,
	cache_functions,
	PHP_MINIT(cache),
	PHP_RINIT(cache),
	PHP_RSHUTDOWN(cache),
	PHP_MSHUTDOWN(cache),
	NULL,
#if ZEND_MODULE_API_NO >= 20010901
	PHP_CACHE_VERSION,
#endif
	STANDARD_MODULE_PROPERTIES
};

ZEND_GET_MODULE(cache)

#if ZEND_MODULE_API_NO <= 20131226
#define _RETURN_STRING(str) RETURN_STRING(str, 0)
typedef long zend_long;
typedef int strsize_t;
#else
#define _RETURN_STRING(str) RETURN_STRING(str)
typedef size_t strsize_t;
#endif

/*PHP_INI_BEGIN()
PHP_INI_ENTRY("cache.test", "14", PHP_INI_ALL, NULL) // FIX THIS
PHP_INI_ENTRY("cache.shdir", "/usr/lib/abadon/mgr", PHP_INI_ALL, NULL) // AND THIS
PHP_INI_END()*/

PHP_MINIT_FUNCTION(cache)
{
	//REGISTER_INI_ENTRIES();
}

PHP_RINIT_FUNCTION(cache)
{
	//REGISTER_INI_ENTRIES();
	//CacheSetDir(INI_STR("cache.shdir"));
}

PHP_RSHUTDOWN_FUNCTION(cache)
{
	//CacheEnd();
}

PHP_MSHUTDOWN_FUNCTION(cache)
{
	//UNREGISTER_INI_ENTRIES();
	CacheEnd();
}

PHP_FUNCTION(cach_set_dir)
{
	strsize_t cparams;
	int temp = CACHE_NO_ERROR;
	char *path;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &cparams) == FAILURE) {
		temp = CACHE_ERROR;
	} else {
		if (0 != CacheSetDir(path)) temp = CACHE_ERROR;
	}
	RETURN_LONG(temp);
}

PHP_FUNCTION(cach_connect)
{
	strsize_t argument_count, username_len, password_len;
	int res = CACHE_NO_ERROR, tout = 0;
	char *username, *password;
	CACHE_STR pusername, ppassword, pexename;
	if (zend_parse_parameters(2, "ss|",
		&username, &username_len, &password, &password_len) == FAILURE) {
		res = CACHE_ERROR;
	} else {
		argument_count = ZEND_NUM_ARGS();
		if (2 == argument_count || 3 == argument_count) {
			strcpy((char *) pexename.str,"php");
			pexename.len = (unsigned short)strlen((char *) pexename.str);

			strcpy((char *) pusername.str,(char *) username);
			pusername.len = username_len;

			strcpy((char *) ppassword.str,(char *) password);
			ppassword.len = password_len;

			if (3 == argument_count) {

				#if ZEND_MODULE_API_NO <= 20131226
					zval **args[3];
					if(zend_get_parameters_array_ex(argument_count, args) != SUCCESS) {
						res = CACHE_ERROR;
					} else {
						if (PZVAL_IS_REF(*args[2])) {
							res = CACHE_ERROR;
						} else {
							if(IS_LONG == Z_TYPE_P(*args[2])) {
								tout = Z_LVAL_P(*args[2]);
							} else {
								res = CACHE_ERROR;
							}
						}
					}
				#else
					zval args[3];
					if(zend_get_parameters_array_ex(argument_count, args) != SUCCESS) {
						res = CACHE_ERROR;
					} else {
						if(IS_LONG == Z_TYPE(args[2])) {
							tout = Z_LVAL(args[2]);
						} else {
							res = CACHE_ERROR;
						}
					}
				#endif

			}
			if (res) {
				/* CACHE_PROGMODE т.к в случае ошибки соединение обрывается и не работает CacheError,
				CACHE_TTNONE чтобы не перехватывало I/O */
				if (0 != CacheSecureStart(&pusername,&ppassword,&pexename,CACHE_PROGMODE|CACHE_TTNONE,tout,NULL,NULL))
					res = CACHE_ERROR;
			}
		} else {
			res = CACHE_ERROR;
		}
	}
	RETURN_LONG(res);
}

PHP_FUNCTION(cach_quit)
{
	RETURN_LONG(0 == CacheEnd());
}

static int __push_zval(zval* value)
{
	int res = CACHE_NO_ERROR;
	switch (Z_TYPE_P(value)) {
		case IS_NULL:	
			res = CACHE_ERROR;
			break;

		#if ZEND_MODULE_API_NO <= 20131226
			case IS_BOOL:
				if (0 != CachePushInt(Z_LVAL_P(value))) res = CACHE_ERROR;
				break;
		#endif

		case IS_LONG:
			if (0 != CachePushInt64(Z_LVAL_P(value))) res = CACHE_ERROR;
			break;

		case IS_DOUBLE:
			if (0 != CachePushDbl(Z_DVAL_P(value))) res = CACHE_ERROR;
			break;

		case IS_STRING:
			if (0 != CachePushStr(Z_STRLEN_P(value),Z_STRVAL_P(value))) res = CACHE_ERROR;
			break;

		default:
			res = CACHE_ERROR;
			break;
	}
	return res;
}

static int __push_pp_global(int argument_count)
{
	char *name;
	strsize_t name_len;
	int res = CACHE_NO_ERROR;
	if (zend_parse_parameters(1, "s|", &name, &name_len) == FAILURE) {
		res = CACHE_ERROR;
	} else {
		if (1 > argument_count || 12 < argument_count) {
			res = CACHE_ERROR;
		} else {
			#if ZEND_MODULE_API_NO <= 20131226
				zval ***args[12];
				if(zend_get_parameters_array_ex(argument_count, args) != SUCCESS) {
					res = CACHE_ERROR;
				} else {
					if ((0 != CachePushGlobal(name_len,name))) {
						res = CACHE_ERROR;
					} else {
						int counter;
						for (counter = 1; counter < argument_count; counter++) {
							if (PZVAL_IS_REF(*args[counter])) {
								res = CACHE_ERROR;
								break;
							} else {
								if(!(res = __push_zval(*args[counter]))) break;
							}
						}
					} 
				}
			#else
				zval args[12];
				if(zend_get_parameters_array_ex(argument_count, args) != SUCCESS) {
					res = CACHE_ERROR;
				} else {
					if (0 != CachePushGlobal(name_len,name)) {
						res = CACHE_ERROR;
					} else {
						int counter;
						for (counter = 1; counter < argument_count; counter++) {
							if(!(res = __push_zval(&args[counter]))) break;
						}
					}
				}
			#endif

		}
	}
	return res;
}

PHP_FUNCTION(cach_set)
{
	int res = CACHE_NO_ERROR;
	if (1 < ZEND_NUM_ARGS()) {
		if(res = __push_pp_global(ZEND_NUM_ARGS())) {
			if (0 != CacheGlobalSet(ZEND_NUM_ARGS()-2)) res = CACHE_ERROR;
		}
	} else {
		res = CACHE_ERROR;
	}
	RETURN_LONG(res);
}

PHP_FUNCTION(cach_get)
{
	int flag = 1, res;
	if (res = __push_pp_global(ZEND_NUM_ARGS())) {
		if (0 != CacheGlobalGet(ZEND_NUM_ARGS()-1,flag)) {
			res = CACHE_ERROR;
		} else {
			int iTemp;
			double dTemp;
			Callin_char_t *sPTemp;
			switch (CacheType()) {
				case CACHE_INT:
					if(0 != CachePopInt(&iTemp)) {
						res = CACHE_ERROR;
					} else {
						RETURN_LONG(iTemp);
					}
					break;

				case CACHE_IEEE_DBL: //Function "CachePopIEEEDbl" is missing
					/* fall-through */
				case CACHE_DOUBLE:
					if(0 != CachePopDbl(&dTemp)) {
						res = CACHE_ERROR;
					} else {
						RETURN_DOUBLE(dTemp);
					}
					break;

				case CACHE_ASTRING:
					/* fall-through */
				case CACHE_WSTRING:
					if(0 != CachePopStr(&iTemp, &sPTemp)) {
						res = CACHE_ERROR;
					} else {
						sPTemp[iTemp]='\0';
						RETURN_STRING(sPTemp);
					}
					break;

				default:
					res = CACHE_ERROR;
					break;
			}
		}
	}
	RETURN_LONG(res);
}

PHP_FUNCTION(cach_kill)
{
	int res, argc = ZEND_NUM_ARGS();
	if (res = __push_pp_global(argc)) {
		if (0 != CacheGlobalKill(argc-1,1)) {
			res = CACHE_ERROR;
		}
	}
	RETURN_LONG(res);
}

PHP_FUNCTION(cach_kill_tree)
{
	int res, argc = ZEND_NUM_ARGS();
	if (res = __push_pp_global(argc)) {
		if(0 != CacheGlobalKill(argc-1,0)) {
			res = CACHE_ERROR;
		}
	}
	RETURN_LONG(res);
}

PHP_FUNCTION(cach_order)
{
	int res, argc = ZEND_NUM_ARGS();
	if (res = __push_pp_global(argc)) {
		if (0 != CacheGlobalOrder(argc-1,1,0)) {
			res = CACHE_ERROR;
		} else {
			int iTemp;
			double dTemp;
			Callin_char_t *sPTemp;
			switch (CacheType()) {
				case CACHE_INT:
					if(0 != CachePopInt(&iTemp)) {
						res = CACHE_ERROR;
					} else {
						RETURN_LONG(iTemp);
					}
					break;

				case CACHE_IEEE_DBL: //Function "CachePopIEEEDbl" is missing
					/* fall-through */
				case CACHE_DOUBLE:
					if(0 != CachePopDbl(&dTemp)) {
						res = CACHE_ERROR;
					} else {
						RETURN_DOUBLE(dTemp);
					}
					break;

				case CACHE_ASTRING:
					/* fall-through */
				case CACHE_WSTRING:
					if(0 != CachePopStr(&iTemp, &sPTemp)) {
						res = CACHE_ERROR;
					} else {
						sPTemp[iTemp]='\0';
						RETURN_STRING(sPTemp);
					}
					break;

				default:
					res = CACHE_ERROR;
					break;
			}
		}
	}
	RETURN_LONG(res);
}

PHP_FUNCTION(cach_order_rev)
{
	int res, argc = ZEND_NUM_ARGS();
	if (res = __push_pp_global(argc)) {
		if(0 != CacheGlobalOrder(argc-1,-1,0)) {
			res = CACHE_ERROR;
		} else {
			int iTemp;
			double dTemp;
			Callin_char_t *sPTemp;
			switch (CacheType()) {
				case CACHE_INT:
					if(0 != CachePopInt(&iTemp)) {
						res = CACHE_ERROR;
					} else {
						RETURN_LONG(iTemp);
					}
					break;

				case CACHE_IEEE_DBL: //Function "CachePopIEEEDbl" is missing
					/* fall-through */
				case CACHE_DOUBLE:
					if(0 != CachePopDbl(&dTemp)) {
						res = CACHE_ERROR;
					} else {
						RETURN_DOUBLE(dTemp);
					}
					break;

				case CACHE_ASTRING:
					/* fall-through */
				case CACHE_WSTRING:
					if(0 != CachePopStr(&iTemp, &sPTemp)) {
						res = CACHE_ERROR;
					} else {
						sPTemp[iTemp]='\0';
						RETURN_STRING(sPTemp);
					}
					break;

				default:
					res = CACHE_ERROR;
					break;
			}
		}
	}
	RETURN_LONG(res);	
}

PHP_FUNCTION(cach_exec)
{
	char *command_str;
	int res = CACHE_NO_ERROR;
	strsize_t command_len;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &command_str, &command_len) == FAILURE) { 
		res = CACHE_ERROR;
	} else {
		CACHE_STR command;
		strcpy((char *) command.str,command_str);
		command.len = command_len;
		if (0 != CacheExecute(&command)) {
			res = CACHE_ERROR;
		}
	}
	RETURN_LONG(res);
}

/*****************************************************************************************/

PHP_FUNCTION(test)
{

}