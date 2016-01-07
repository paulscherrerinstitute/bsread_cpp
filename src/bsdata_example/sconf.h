/*
 Simple configuration utility library for C and C++

 Copyright (c) 2015, Tom Slejko
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UTIL_SCONF_H_GUARD
#define UTIL_SCONF_H_GUARD


#include <string.h>
#include <stdlib.h>
#include <stdio.h>

enum conf_item_type {CONF_STRING,CONF_INT,CONF_DOUBLE,CONF_BOOL};
const char* conf_item_type_name[] = {"string","int","double","bool"};


struct conf_string_t{
    char** target;
    const char* def;
};

struct conf_int_t{
    int* target;
    int min;
    int max;
    int def;
};

struct conf_double_t{
    double* target;
    double min;
    double max;
    double def;
};

struct conf_bool_t{
    char* target;
    char def;
};

struct conf_item_t{
    const char* name;
    const char* help;
    conf_item_type type;
    void* type_pvt;
    conf_item_t* next;
};

inline conf_item_t* conf_create_item(conf_item_t* root, const char* name, const char* help){
    conf_item_t* item = (conf_item_t*) malloc(sizeof(conf_item_t));
    item->name = name;
    item->help = help;

    if(root){
        while(root->next) root=root->next;
        root->next=item;
    }

    item->next=NULL;

    return item;
}

conf_item_t* conf_create_stri_item(conf_item_t* root, char** target, const char* name, const char* def, const char* help){
    conf_item_t* item = conf_create_item(root, name,help);

    conf_string_t* pvt = (conf_string_t*) malloc(sizeof(conf_string_t));

    pvt->def = def;
    pvt->target = target;
    *pvt->target = strdup(def);

    item->type_pvt = pvt;
    item->type = CONF_STRING;
    return item;
}

conf_item_t* conf_create_bool_item(conf_item_t* root, char* target, const char* name, char def, const char* help){
    conf_item_t* item = conf_create_item(root,name,help);
    conf_bool_t* pvt = (conf_bool_t*) malloc(sizeof(conf_bool_t));

    pvt->def = def;
    pvt->target = target;
    *pvt->target = def;

    item->type_pvt = pvt;
    item->type = CONF_BOOL;
    return item;
}

conf_item_t* conf_create_double_item(conf_item_t* root, double* target, const char* name, double def, const char* help){
    conf_item_t* item = conf_create_item(root,name,help);
    conf_double_t* pvt = (conf_double_t*) malloc(sizeof(conf_double_t));

    pvt->def = def;
    pvt->target = target;
    *pvt->target = def;

    item->type_pvt = pvt;
    item->type = CONF_DOUBLE;
    return item;
}

/**
 * @brief conf_create_int_item Create integer configruation parameter
 * @param root root or previous node of configuration linked list to which the new parameter should be appended.
 * @param target pointer to target variable
 * @param name unique parameter name
 * @param def default value
 * @param help help string (can be 0)
 * @return
 */
conf_item_t* conf_create_int_item(conf_item_t* root, int* target, const char* name, int def, const char* help){
    conf_item_t* item = conf_create_item(root,name,help);
    conf_int_t* pvt = (conf_int_t*) malloc(sizeof(conf_int_t));

    pvt->def = def;
    pvt->target = target;
    *pvt->target = def;

    item->type_pvt = pvt;
    item->type = CONF_INT;
    return item;
}

/**
 * @brief conf_free deletes whole memory structure
 * @param root
 */
void conf_free(conf_item_t* root){
    conf_item_t* next;
    while(root){
        next=root->next;
        free(root->type_pvt);
        free(root);
        root=next;
    }
}

/**
 * @brief conf_find Find configuration item by name
 * @param root root configuration node
 * @param name name of item beeing looked for
 * @return
 */
conf_item_t* conf_find(conf_item_t* root, const char* name){
    while(root){
        if(!strcmp(root->name,name)){
            return root;
        }
        root=root->next;
    }

    return NULL;
}

/**
 * @brief conf_set Convert input value string to correct type and set it to the conf_item
 * @param item
 * @param value
 */
void conf_set(conf_item_t* item, const char* value){
    if(item){
        switch(item->type){
            case CONF_STRING:
            {
                free(*((conf_string_t*)(item->type_pvt))->target);

                if(value[0]=='#') *((conf_string_t*)(item->type_pvt))->target = 0;
                else *((conf_string_t*)(item->type_pvt))->target=strdup(value);

                break;
            }
            case CONF_BOOL:
            {
                int val = strtol(value,0,0);
                *((conf_bool_t*)(item->type_pvt))->target = val != 0 ? 1:0;
                break;
            }
            case CONF_INT:
            {
                int val = strtol(value,0,0);
                *((conf_int_t*)(item->type_pvt))->target=val;
                break;
            }
            case CONF_DOUBLE:
            {
                double val = strtod(value,0);
                *((conf_double_t*)(item->type_pvt))->target=val;
                break;
            }
        }
    }
}

/**
 * @brief conf_get
 * @param root
 * @return string represnetation of configuration item, note that the caller is responsible for freeing the memory!
 *          Returns NULL if root is NULL or type is invalid
 */
char* conf_get(conf_item_t* root){
    char* val=0;
    if(root){
        switch(root->type){
            case CONF_STRING:
            {
                asprintf(&val,"%s",*((conf_string_t*)(root->type_pvt))->target);
                break;
            }
            case CONF_BOOL:
            {
                asprintf(&val,"%d",*((conf_bool_t*)(root->type_pvt))->target != 0 ? 1: 0);
                break;
            }
            case CONF_INT:
            {
                asprintf(&val,"%d",*((conf_int_t*)(root->type_pvt))->target);
                break;
            }
            case CONF_DOUBLE:
            {
                asprintf(&val,"%f",*((conf_double_t*)(root->type_pvt))->target);
                break;
            }
        }
    }

    return val;
}

/**
 * @brief conf_saves Save value of variable name to string target. If string
 * is not found target is set to 0 length string
 * @param target
 * @param name
 */
char* conf_gets(const char* name, conf_item_t* root){
    conf_item_t* item = conf_find(root,name);
    char* out;
    if(!item){
        return 0;
    }

    char* value  = conf_get(item);
    asprintf(&out,"%s %s",name,value);
    free(value);
    return out;

}


/**
 * @brief conf_save Generate configuration file to desired output stream
 * @param f
 * @param root
 */
void conf_savef(FILE* f,conf_item_t* root){
    while(root){
        char* val = conf_get(root);

        int len = fprintf(f,"%s %*s%s",root->name,30-strlen(root->name),"",val);

        free(val);

        fprintf(f, "%*s# type: %s",50-len,"",conf_item_type_name[root->type]);
        if(root->help){
            fprintf(f," desc: %s",root->help);
        }

        fprintf(f,"\n");
        root=root->next;
    }
}

/**
 * @brief conf_savefs attempt to save configuration to a file
 * @param filename
 * @param root
 * @return 0 on success, 1 on error
 */
int conf_savefs(const char* filename, conf_item_t* root){
    FILE* f = fopen(filename,"w+");
    if(!f) return 1;
    conf_savef(f,root);
    fclose(f);
}


/**
 * @brief conf_loads Load configuration string (e..g set 1 item)
 * @param string
 * @param root
 */
void conf_loads(const char* string, conf_item_t* root){
    char* name;
    char* value;

    if(sscanf(string,"%as %as",&name,&value)!=2)
        return;

    root = conf_find(root,name);
    if(root)
        conf_set(root,value);

    free(name);
    free(value);
}

void conf_loadf(FILE* f,conf_item_t* root){
    char linebuffer[1024];
    while(fgets(linebuffer,1024,f))
        conf_loads(linebuffer,root);
}

int conf_load_cmdline(int argc, char* argv[], conf_item_t* root){
    int err=0;

    //Trim the odd argument
    if(! (argc % 2) ) argc--;

    int i=1;
    while(i<argc){
        char* name = argv[i++];
        char* val = argv[i++];

        conf_item_t* item = conf_find(root,name);
        if(!item){
            fprintf(stderr,"Warning: argument %s not supported!\n",name);
            err=1;
        }

        conf_set(item,val);
    }
}

/**
 * @brief conf_loadfs attempt to load configuration from a file
 * @param filename
 * @param root
 * @return 0 on sucess, 1 if file does not exist or cannot be opened
 */
int conf_loadfs(const char* filename, conf_item_t* root){
    FILE* f = fopen(filename,"r");
    if(!f) return 1;
    conf_loadf(f,root);
    fclose(f);
}

void conf_dump(conf_item_t* root){
    while(root){
        char* value = conf_get(root);
        printf("%s of type %s, current value: %s\n",root->name,conf_item_type_name[root->type],value);
        free(value);
        root=root->next;
    }
}


#endif
