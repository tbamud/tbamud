/*
jsIOn.h
JavaScript (Input/Output Object) Notation

Copyright 2022 Joseph Arnusch
https://github.com/JosephVTK/jsIOn

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "jsIOn.h"

/*
Functions to be used outside of this file.
*/

/* Creators */
JSONdata *jsonCreateArray(const char *key);
JSONdata *jsonCreateObject(const char *key);
JSONdata *jsonCreateString(const char *key, const char *variable);
JSONdata *jsonCreateInt(const char *key, int variable);
JSONdata *jsonCreateDouble(const char *key, double variable);
JSONdata *jsonCreateBool(const char *key, int variable);

/* Modifiers */
JSONdata *jsonAddObject(JSONdata *container, JSONdata *item);
JSONdata *jsonGetValueFromObject(JSONdata *object, const char *key);

/* Utilities */
JSONdata *json_read_from_disk(const char *file_name);
int json_free_object(JSONdata *j);
void json_write_to_disk(const char *file_name, JSONdata *object);
int jsonKeyIs(JSONdata *object, const char *key);

// ----------------------

// Local
static void parse_json_string(char **ptr_string, JSONdata *parent);


/*
    I needed a function to find the location of the last quotation mark " in a string
    and thus this is it.
*/
static int reverse_count_to_char(const char *string, char to_char) {
    ssize_t len = strlen(string);
    int i;

    for (i = len; i > 0; i--) {
        if (string[i] == to_char)
            return i;
    }

    return -1;
}

/*
    Prep new JSONdata for use and assign a key if required.
*/
static void _set_new_json(JSONdata *j, const char *key) {
    j->key = key ? strdup(key) : NULL;
    j->json_value_type = jsonNULL;

    j->child = NULL;
    j->next = NULL;
    j->last = NULL;

    j->j_string = NULL;
    j->j_double = 0.00;
    j->j_integer = 0;
}

/*
    Allocate the actual JSONdata and send it for prep.
*/
static JSONdata *_jsonCreate(const char *key) {
    JSONdata *new_json;
    new_json = (JSONdata *)malloc(sizeof(JSONdata));
    _set_new_json(new_json, key);

    return new_json;
}

/*
    Free all allocated memory created during our json process.
*/
int json_free_object(JSONdata *j) {
    JSONdata *tmp;
    int i = 0;

    while (j->child != NULL) {
        tmp = j->child;
        j->child = j->child->next;
        i += json_free_object(tmp);
    }

    j->child = NULL;
    j->next = NULL;
    j->last = NULL;

    if (j->key)
        free(j->key);

    if (j->j_string)
        free(j->j_string);

    j->json_value_type = jsonNULL;

    free(j);
    i += 1;

    return i;
}

/*
    Create a new json object and set it's type.
*/
JSONdata *jsonCreateObject(const char *key) {
    JSONdata *new_json;

    new_json = _jsonCreate(key);
    new_json->json_value_type = jsonOBJECT;

    return new_json;
}

/*
    See jsonCreateObject()
*/
JSONdata *jsonCreateArray(const char *key) {
    JSONdata *new_json;

    new_json = _jsonCreate(key);
    new_json->json_value_type = jsonARRAY;

    return new_json;
}

/*
    See jsonCreateObject()
*/
JSONdata *jsonCreateInt(const char *key, int variable) {
    JSONdata *new_json;

    new_json = _jsonCreate(key);

    new_json->json_value_type = jsonINT;
    new_json->j_integer = variable;

    return new_json;
}

/*
    See jsonCreateObject()
*/
JSONdata *jsonCreateDouble(const char *key, double variable) {
    JSONdata *new_json;

    new_json = _jsonCreate(key);

    new_json->json_value_type = jsonDOUBLE;
    new_json->j_double = variable;

    return new_json;
}

/*
    See jsonCreateObject()
    ...

    I've made the design decision to not use the boolean type and just repurpose
    our integer variable as the boolean using a simple 1 or 0.
*/
JSONdata *jsonCreateBool(const char *key, int variable) {
    JSONdata *new_json;

    new_json = _jsonCreate(key);

    // TODO: Safety check that variable is infact 1 or 0

    new_json->json_value_type = jsonBOOL;
    new_json->j_integer = variable;

    return new_json;
}

/*
    See jsonCreateObject()
*/
JSONdata *jsonCreateString(const char *key, const char *variable) {
    JSONdata *new_json;

    new_json = _jsonCreate(key);

    new_json->json_value_type = jsonSTRING;
    new_json->j_string = strdup(variable);

    return new_json;
}

/*
    Here we are adding an item to our json container whether that container be an array or object.
    We return the inserted item to allow for one line additions.
*/
JSONdata *jsonAddObject(JSONdata *container, JSONdata *item) {

    if (container->last) {
        container->last->next = item;
        container->last = item;
    } else {
        container->child = item;
        container->last = item;
    }

    return item;
}


/*
    Retrieve a value from an object
*/
JSONdata *jsonGetValueFromObject(JSONdata *object, const char *key) {
    JSONdata *j;

    if (IS_JSON(object, jsonOBJECT) == FALSE)
        return NULL;

    for (j = object->child; j != NULL; j = j->next) {
        if (j->key == NULL)
            continue;
     
        if (strcmp(j->key, key) == 0)
            return j;
    }

    return NULL;
}

int jsonKeyIs(JSONdata *object, const char *key) {
    return 1 ? strcmp(key, object->key) == 0 : 0;
}

/*
    Here we are moving through the submitted string and grabbing the contents between quotation marks.

    TODO: This needs to be fleshed out. I have concerns about operation if a " finds it's wait inside of a key. Also proper
    error handling would be nice.
*/
const char *get_json_key(char **ptr_string) {
    static char temp[JSON_MAX_KEY_BUFFER];
    *temp = '\0';
    int in_key = FALSE;

    for (; **ptr_string != '\0'; ++ * ptr_string) {

        if (**ptr_string == '\\') {
            /*

                Add the \ character and the next character as well.

            */
            sprintf(temp + strlen(temp), "%c", **ptr_string);
            ++ *ptr_string;
            sprintf(temp + strlen(temp), "%c", **ptr_string);
            continue;
        }

        if (**ptr_string == '"') {
            if (in_key == FALSE) {
                in_key = TRUE;
                continue;
            } else {
                in_key = FALSE;
                break;
            }
        }
        sprintf(temp + strlen(temp), "%c", **ptr_string);
    }

    return temp;
}

/*
    Here we are moving through the submitted key and grabbing the contents.

    We attempt to differentiate between different value types.

    If we encounter an { or [ not within a string, we assume we have a new object/array and we use the power
    of recursion to go back to the beginning and collect for this object.

    TODO: This needs to be fleshed out. I have concerns about operation if a " finds it's wait inside of a value. Also proper
    error handling would be nice.
*/
void parse_json_value(char **ptr_string, JSONdata *new_object) {
    static char temp[JSON_MAX_VALUE_BUFFER];
    *temp = '\0';

    int i;
    int in_string = FALSE;
    int is_int = FALSE;
    int is_double = FALSE;
    int is_string = FALSE;
    int is_bool = -1;

    for (; **ptr_string != '\0'; ++ * ptr_string) {
        if (**ptr_string == '"')
            in_string = !in_string;

        if (in_string == FALSE) {
            if (**ptr_string == '{') {
                ++ *ptr_string;
                new_object->json_value_type = jsonOBJECT;
                parse_json_string(ptr_string, new_object);
                return;
            }

            if (**ptr_string == '[') {
                ++ *ptr_string;
                new_object->json_value_type = jsonARRAY;
                parse_json_string(ptr_string, new_object);
                return;
            }

            if (**ptr_string == '}' || **ptr_string == ']' || **ptr_string == ',' || **ptr_string == ' ')
                break;
        } else {
            if (**ptr_string == '\\') {
                /*

                    Add the \ character and the next character as well.

                */
                sprintf(temp + strlen(temp), "%c", **ptr_string);
                ++ *ptr_string;
                sprintf(temp + strlen(temp), "%c", **ptr_string);
                continue;
            }
        }


        snprintf(temp + strlen(temp), JSON_MAX_VALUE_BUFFER, "%c", **ptr_string);
    }

    if (temp[0] == '"') {
        is_string = TRUE;
    } else {

        if (strcasecmp(temp, "true") == 0) {
            is_bool = TRUE;
        } else if (strcasecmp(temp, "false") == 0) {
            is_bool = FALSE;
        } else {
            is_int = TRUE;
            is_double = TRUE;

            /* Is it an integer or just a plain zero */
            for (i = 0; i < strlen(temp); i++) {
                if (isdigit(temp[i]) == 0 && isspace(temp[i]) == 0) {
                    is_int = FALSE;
                    if (temp[i] != '.')
                        is_double = FALSE;
                }
            }
        }
    }

    if (is_string == TRUE) {
        /* TODO: This needs to be safer */
        new_object->j_string = strndup(temp + 1, reverse_count_to_char(temp, '"')-1);
        new_object->json_value_type = jsonSTRING;
    } else if (is_bool >= 0) {
        new_object->j_integer = is_bool;
        new_object->json_value_type = jsonBOOL;
    } else if (is_int == TRUE) {
        new_object->j_integer = atoi(temp);
        new_object->json_value_type = jsonINT;
    } else if (is_double == TRUE) {
        new_object->j_double = strtod(temp, NULL);
        new_object->json_value_type = jsonDOUBLE;

    }

}

/*

    This is the master parser which starts at the beginning on a new json string.

*/
static void parse_json_string(char **ptr_string, JSONdata *parent) {
    JSONdata *new_object = NULL;

    for (; **ptr_string != '\0'; ++ * ptr_string) {

        if (isspace(**ptr_string) || **ptr_string == ',')
            continue;

        if (IS_JSON(parent, jsonOBJECT) && **ptr_string == '}') {
            ++ *ptr_string;
            break;
        }
        if (IS_JSON(parent, jsonARRAY) && **ptr_string == ']') {
            ++ *ptr_string;
            break;
        }

        if (new_object == NULL && parent->json_value_type == jsonOBJECT) {
            if (parent->json_value_type == jsonOBJECT) {
                new_object = jsonCreateObject(get_json_key(ptr_string));
                jsonAddObject(parent, new_object);
            }
        } else {
            if (parent->json_value_type == jsonOBJECT) {
                if (**ptr_string == ':') {
                    while (**ptr_string != '\0') {
                        if (**ptr_string == ':' || isspace(**ptr_string))
                            ++ *ptr_string;
                        else
                            break;
                    }
                    parse_json_value(ptr_string, new_object);
                    new_object = NULL;
                }
            } else if (parent->json_value_type == jsonARRAY) {
                new_object = jsonCreateObject(NULL);
                jsonAddObject(parent, new_object);
                parse_json_value(ptr_string, new_object);
                new_object = NULL;
            }
        }

        /*
        If somewhere along the way we've come to the end of the string, break the loop.
        If we allow the loop to iterate we wander off the memory trail and you will see things not
        meant for mortal eyes... BEWARE!!! */

        if (**ptr_string == '\0')
            break;

        if (IS_JSON(parent, jsonOBJECT) && **ptr_string == '}') {
            ++ *ptr_string;
            break;
        }
        if (IS_JSON(parent, jsonARRAY) && **ptr_string == ']') {
            ++ *ptr_string;
            break;
        }

    }
}

/*

    This is controller which feeds the master parser.

*/

JSONdata *get_json_from_string(char **ptr_string) {
    JSONdata *parent_object = NULL;

    for (;**ptr_string != '\0'; ++ * ptr_string) {
        if (isspace(**ptr_string))
            continue;

        if (**ptr_string == '{' || **ptr_string == '[')
            break;
    }

    if (**ptr_string == '{') {
        ++ *ptr_string; // Bump our index up 1 so we bypass the '{' in the string.
        parent_object = jsonCreateObject(NULL);
        parse_json_string(ptr_string, parent_object);
    } else if (**ptr_string == '[') {
        ++ *ptr_string;
        parent_object = jsonCreateArray(NULL);
        parse_json_string(ptr_string, parent_object);
    }
    return parent_object;

}

/*

    Provides a string representation of a json value

*/

static char *json_value_to_string(JSONdata *item) {
    static char buf[256];

    switch (item->json_value_type)
    {
    case jsonARRAY:
        return NULL;
    case jsonBOOL:
        sprintf(buf, "%s", item->j_integer == 1 ? "true" : "false");
        return buf;
    case jsonINT:
        sprintf(buf, "%d", item->j_integer);
        return buf;
    case jsonDOUBLE:
        sprintf(buf, "%f", item->j_double);
        return buf;
    case jsonSTRING:
        sprintf(buf, "\"%s\"", item->j_string);
        return buf;
    default:
        return '\0';
    }
}

/*

    Prints the contents of a json object and it's dependants into a buffer.
    Primarily for file writing.

*/

void json_to_buf(JSONdata *object, char *buf, ssize_t max_len, int indent) {
    JSONdata *obj;
    int i;
    char indent_buffer[8];
    *indent_buffer = '\0';

    for (i = 0; i < indent; i++)
        snprintf(indent_buffer + strlen(indent_buffer), 8, JSON_BUFFER_STRING);

    if (object->json_value_type == jsonARRAY) {
        if (object->key == NULL)
            snprintf(buf + strlen(buf), max_len, "%s[\n", indent_buffer);
        else
            snprintf(buf + strlen(buf), max_len, "%s\"%s\" : [\n", indent_buffer, object->key);
    } else if (object->json_value_type == jsonOBJECT) {
        if (object->key == NULL)
            snprintf(buf + strlen(buf), max_len, "%s{\n", indent_buffer);
        else
            snprintf(buf + strlen(buf), max_len, "%s\"%s\" : {\n", indent_buffer, object->key);
    }
    for (obj = object->child; obj != NULL; obj = obj->next) {
        if (obj->json_value_type == jsonARRAY || obj->json_value_type == jsonOBJECT) {
            json_to_buf(obj, buf, max_len, indent + 1);

        } else {
            if (object->json_value_type == jsonOBJECT)
                snprintf(buf + strlen(buf), max_len, "%s%s\"%s\":%s%s\n", indent_buffer, JSON_BUFFER_STRING, obj->key, json_value_to_string(obj), obj->next ? "," : "");
            else
                snprintf(buf + strlen(buf), max_len, "%s%s%s%s\n", indent_buffer, JSON_BUFFER_STRING, json_value_to_string(obj), obj->next ? "," : "");
        }

    }

    if (object->json_value_type == jsonARRAY) {
        snprintf(buf + strlen(buf), max_len, "%s]%s\n", indent_buffer, object->next ? "," : "");
    } else if (object->json_value_type == jsonOBJECT) {
        snprintf(buf + strlen(buf), max_len, "%s}%s\n", indent_buffer, object->next ? "," : "");
    }

}

void json_write_to_disk(const char *file_name, JSONdata *object) {
    char buf[500000];
    FILE *fp;

    json_to_buf(object, &*buf, 65535, 0);
    fp = fopen(file_name, "w+");
    fprintf(fp, "%s", buf);
    fclose(fp);
}

JSONdata *json_read_from_disk(const char *file_name) {
    FILE *json_file;
    char *string;
    char *to_json;
    long numbytes;
    JSONdata *object;

    json_file = fopen(file_name, "r");
    if (json_file == NULL)
        return NULL;

    fseek(json_file, 0L, SEEK_END);
    numbytes = ftell(json_file);
    fseek(json_file, 0L, SEEK_SET);

    string = (char *)calloc(numbytes, sizeof(char));
    if (string == NULL)
        return NULL;

    if (fread(string, sizeof(char), numbytes, json_file) != numbytes) {
        printf("Something went wrong reading the file.\n");
        exit(1);
    }
    
    fclose(json_file);

    to_json = string;

    object = get_json_from_string(&to_json);

    free(string);

    return object;
}