#include <stdbool.h>
#include <stdlib.h>
#include "hash.h"
#include "lista.h"
#include <string.h>

#define CAMPOS_INICIAL 13
#define COEFICIENTE_AUM 1
#define COEFICIENTE_RED 0.1
#define FACTOR_AUM true
#define FACTOR_RED false

/* 
HASH FUNC: MURMURHASH
https://enqueuezero.com/algorithms/murmur-hash.html
CREADO POR AUSTIN APPLEBY, OPTIMIZADO POR NGINX
*/

int ngx_murmur_hash2(const char* data, int len) {
    int h, k;

    h = 0 ^ len;

    while (len >= 4) {
        k = data[0];
        k |= data[1] << 8;
        k |= data[2] << 16;
        k |= data[3] << 24;

        k *= 0x5bd1e995;
        k ^= k >> 24;
        k *= 0x5bd1e995;

        h *= 0x5bd1e995;
        h ^= k;

        data += 4;
        len -= 4;
    }

    switch (len) {
        case 3:
            h ^= data[2] << 16;
            /* fall through */
        case 2:
            h ^= data[1] << 8;
            /* fall through */
        case 1:
            h ^= data[0];
            h *= 0x5bd1e995;
    }

    h ^= h >> 13;
    h *= 0x5bd1e995;
    h ^= h >> 15;

    return h;
}

/*
HASH ABIERTO
*/
struct hash{
    lista_t** campos;
    void (*hash_destruir_dato_t)(void*);
    int cant_campos;
    size_t cant_claves;
};

int f_hash(const char* clave, int cant_campos){
    return ngx_murmur_hash2(clave, (int)strlen(clave)) % cant_campos;
}

bool _iniciar_listas(lista_t** arr, int cant) {
    for (int i = 0; i < cant; i++) {
        arr[i] = lista_crear();
        if (!arr[i]) return NULL;
    }
    return true;
}

hash_t* hash_crear(hash_destruir_dato_t destruir_dato){
    hash_t* hash = malloc(sizeof(hash_t));
    hash->campos = malloc(CAMPOS_INICIAL * sizeof(lista_t*));
    hash->cant_campos = CAMPOS_INICIAL;
    if (!_iniciar_listas(hash->campos, hash->cant_campos)) return NULL;
    hash->hash_destruir_dato_t = destruir_dato;
    hash->cant_claves = 0;
    return hash;
}

int _factor_redimension(hash_t* hash, bool factor){
    if (factor) return hash->cant_campos * 2;
    return hash->cant_campos / 2;
}
/*LAS CLAVES SON UNA COPIA DE LAS QUE PASAN, NO GUARDA CLAVE
DIRECTAMENTE PORQUE SI LA CAMBIA EL USUARIO DESDE DONDE LA TIENE GUARDA
DA SE ROMPE TODO, la clave se borra directamente en lista_borrar*/
lista_t** _redimensionar(hash_t* hash, bool factor){
    int cant = _factor_redimension(hash, factor); //ajustar primos
    if (cant == 0) return hash->campos;
    lista_t** aux = malloc(cant * sizeof(lista_t*));
    if (!_iniciar_listas(aux, cant)) return NULL;
    for (int i = 0; i < hash->cant_campos; i++){
        while(!lista_esta_vacia(hash->campos[i])){
            nodo_t* act = hash->campos[i]->prim;
            if (!lista_insertar_ultimo(aux[f_hash(act->clave, cant)], act->dato, act->clave)) return NULL;
            lista_borrar_primero(hash->campos[i]);
        }
        lista_destruir(hash->campos[i], NULL);
    }
    free(hash->campos);
    hash->cant_campos = cant;
    return aux;
}

bool hash_guardar(hash_t* hash, const char* clave, void* dato){
    if ((hash->cant_claves + 1) / (hash->cant_campos) >= COEFICIENTE_AUM){
        hash->campos = _redimensionar(hash, FACTOR_AUM);
    }
    lista_t* lista = hash->campos[f_hash(clave, hash->cant_campos)];
    nodo_t* act = lista->prim;
    while (act){
        if (strcmp(act->clave, clave) == 0) {
            void* aux = act->dato;
            act->dato = dato;
            if (!hash->hash_destruir_dato_t) return true;
            hash->hash_destruir_dato_t(aux);
            return true;
        }
        act = act->prox;
    }
    if (!lista_insertar_ultimo(lista, dato, clave)) return false;
    hash->cant_claves++;
    return true;
}


void* hash_borrar(hash_t* hash, const char* clave){
    if ((hash->cant_claves - 1) / (hash->cant_campos) <= COEFICIENTE_RED){
        hash->campos = _redimensionar(hash, FACTOR_RED);
    }
    lista_t* lista = hash->campos[f_hash(clave, hash->cant_campos)];
    lista_iter_t* iter = lista_iter_crear(lista);
    if (!iter) return NULL;
    while (!lista_iter_al_final(iter)){
        if (strcmp(lista_iter_ver_clave(iter), clave) == 0){
            void* dato = lista_iter_borrar(iter);//la clave se borra adentro la funcion
            hash->cant_claves--;
            lista_iter_destruir(iter);
            return dato;
        }
        lista_iter_avanzar(iter);
    }
    lista_iter_destruir(iter);
    return NULL;
}

void* hash_obtener(const hash_t* hash, const char* clave){
    lista_t* lista = hash->campos[f_hash(clave, hash->cant_campos)];
    nodo_t* act = lista->prim;
    while (act){
        if (strcmp(act->clave, clave) == 0) return act->dato;
        act = act->prox;
    }
    return NULL;
}

bool hash_pertenece(const hash_t* hash, const char* clave){
    lista_t* lista = hash->campos[f_hash(clave, hash->cant_campos)];
    nodo_t* act = lista->prim;
    while (act) {
        if (strcmp(act->clave, clave) == 0) return true;
        act = act->prox;
    }
    return false;
}

size_t hash_cantidad(const hash_t* hash){
    return hash->cant_claves;
}

void hash_destruir(hash_t* hash){
    for(int i = 0; i < hash->cant_campos; i++){
        lista_t* lista = hash->campos[i];
        lista_destruir(lista, hash->hash_destruir_dato_t);
    }
    free(hash->campos);
    free(hash);
}

/* ITERADOR EXTERNO DE TABLA DE HASH ABIERTA*/

typedef struct hash_iter{
    lista_iter_t* iter_lista_act;
    lista_t* lista_act;
    const hash_t* hash;
    int campo;
    int cant_elem_pasados;
} hash_iter_t;

lista_t* _lista_no_vacia(hash_iter_t* iter){
    for (;iter->campo < iter->hash->cant_campos-1; iter->campo++){
        if (!lista_esta_vacia(iter->hash->campos[iter->campo])){
            return iter->hash->campos[iter->campo];
        }
    }
    return iter->hash->campos[iter->campo];
}

hash_iter_t* hash_iter_crear(const hash_t* hash){
    hash_iter_t* iter = malloc(sizeof(hash_iter_t));
    if (!iter) return NULL;
    iter->hash = hash;
    iter->campo = 0;
    iter->cant_elem_pasados = 0;
    iter->lista_act = _lista_no_vacia(iter);
    iter->iter_lista_act = lista_iter_crear(iter->lista_act);
    if (!iter->iter_lista_act) return NULL;
    return iter;
}

bool hash_iter_al_final(const hash_iter_t* iter){
    return iter->cant_elem_pasados == iter->hash->cant_claves;
}

bool hash_iter_avanzar(hash_iter_t* iter){
    if (hash_iter_al_final(iter)) return false;
    if (!lista_iter_al_final(iter->iter_lista_act)){
        if (!lista_iter_avanzar(iter->iter_lista_act)) return false;
        iter->cant_elem_pasados++;
        return true;
    }
    //ahora viene el caso del iter al final
    lista_iter_destruir(iter->iter_lista_act);
    iter->lista_act = _lista_no_vacia(iter);
    iter->iter_lista_act = lista_iter_crear(iter->lista_act);
    iter->cant_elem_pasados++;
    return true;
}

const char* hash_iter_ver_actual(const hash_iter_t* iter){
    return lista_iter_ver_clave(iter->iter_lista_act);
}

void hash_iter_destruir(hash_iter_t* iter){
    free(iter->iter_lista_act);
    free(iter);
}
