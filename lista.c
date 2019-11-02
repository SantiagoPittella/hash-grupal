#include <stdbool.h>
#include <stdlib.h>
#include "lista.h"

/*
LISTA ENLAZADA
*/
nodo_t* nodo_crear(void* dato, const char* clave){
    nodo_t* nodo = malloc(sizeof(nodo_t));
    if (!nodo) return NULL;
    nodo->clave = malloc((strlen(clave) + 1) * sizeof(char));
    strcpy(nodo->clave, clave);
    nodo->dato = dato;
    nodo->prox = NULL;
    return nodo;
}

lista_t* lista_crear() {
    lista_t* lista = malloc(sizeof(lista_t));
    if (!lista) {
        return NULL;
    }
    lista->prim = NULL;
    lista->ult = NULL;
    lista->tam = 0;
    return lista;
}

bool lista_esta_vacia(const lista_t* lista) {
    return lista->tam == 0;
}

bool lista_insertar_primero(lista_t* lista, void* dato, const char* clave){
    nodo_t* aux = nodo_crear(dato, clave);
    if (!aux) return false;
    aux->prox = lista->prim;
    lista->prim = aux;
    if (lista->tam == 0){
        lista->ult = aux;
    }
    lista->tam++;
    return true;
}


bool lista_insertar_ultimo(lista_t* lista, void* dato, const char* clave) {
    nodo_t* aux = nodo_crear(dato, clave);
    if (!aux) return false;
    aux->prox = NULL;
    if (lista->tam == 0) {
        lista->prim = aux;
    } else {
        lista->ult->prox = aux;
    }
    lista->ult = aux;
    lista->tam++;
    return true;
}

void* lista_borrar_primero(lista_t* lista){
    if (lista->tam == 0){
        return NULL;
    }
    nodo_t* aux = lista->prim;
    lista->prim = lista->prim->prox;
    void* dato = aux->dato;
    free(aux->clave);
    free(aux);
    lista->tam--;
    return dato;
}

void lista_destruir(lista_t* lista, void destruir_dato(void*)) {
    while (lista->tam > 0) {
        void* dato = lista_borrar_primero(lista);
        if (destruir_dato != NULL) {
            destruir_dato(dato);
        }
    }
    free(lista);
}

typedef struct lista_iter {
    lista_t* lista;
    nodo_t* act;
    nodo_t* ant;
} lista_iter_t;
/*
ITERADOR EXTERNO
*/
lista_iter_t* lista_iter_crear(lista_t* lista) {
    lista_iter_t* iter = malloc(sizeof(lista_iter_t));
    if (!iter) return NULL;
    iter->lista = lista;
    iter->act = lista->prim;
    iter->ant = lista->prim;
    return iter;
}
/*
*******************************************************************
*                    PRIMITIVA ITERADOR INTERNO                   *
*******************************************************************
*/


bool lista_iter_avanzar(lista_iter_t* iter) {
    if (!iter || !iter->act) return false;
    iter->ant = iter->act;
    iter->act = iter->act->prox;
    return true;
}

void* lista_iter_ver_actual(const lista_iter_t* iter) {
    return iter->act == NULL ? NULL : iter->act->dato;
}
const char* lista_iter_ver_clave(const lista_iter_t* iter){
    return iter->act == NULL ? NULL : iter->act->clave;
}

bool lista_iter_al_final(const lista_iter_t* iter) {
    return iter->act == NULL;
}

void lista_iter_destruir(lista_iter_t* iter) {
    free(iter);
}

bool lista_iter_insertar(lista_iter_t* iter, void* dato, const char* clave) {
    /*tengo lista_insertar_primero y ultimo como primitivas*/
    if (lista_esta_vacia(iter->lista)) {
        lista_insertar_primero(iter->lista, dato, clave);
        iter->act = iter->lista->prim;
        iter->ant = iter->lista->prim;
        //no es necesario subir tam de la lista porque ya lo hace la primitiva
        return true;
    } else if (iter->ant == iter->act) {
        /*si entra aca ya se sabe que la lista no esta vacia*/
        lista_insertar_primero(iter->lista, dato, clave);
        iter->act = iter->lista->prim;
        iter->ant = iter->act;
        return true;
    }
    else if (lista_iter_al_final(iter)) {
        lista_insertar_ultimo(iter->lista, dato, clave);
        iter->act = iter->ant->prox;
        //no es necesario subir tam de la lista porque ya lo hace la primitiva
        return true;
    } else {
        nodo_t* aux = nodo_crear(dato, clave);
        if (!aux) return false;
        iter->ant->prox = aux;
        aux->prox = iter->act;
        iter->lista->tam++;
        iter->act = aux;
        return true;
    }
}
/*usar primitiva de la lista todo lo que pueda*/
void* lista_iter_borrar(lista_iter_t* iter) {
    if (lista_esta_vacia(iter->lista) || lista_iter_al_final(iter)) return NULL;
    if (iter->act == iter->ant) {
        /*si entro aca es seguro que tiene  mas de un elemento, 
        ya que si no hubiese sido contenido en el if anterior*/
        void* dato = lista_borrar_primero(iter->lista);
        iter->act = iter->lista->prim;
        iter->ant = iter->act;  //tanto act como ant apuntan al primer nodo
        return dato;
    }
    void* dato = iter->act->dato;
    nodo_t* aux = iter->act;
    if (!iter->act->prox) {
        iter->act = NULL;
        iter->ant->prox = NULL;
        iter->lista->ult = iter->ant;
        iter->lista->tam--;
        free(aux->clave);
        free(aux);
        return dato;
    }
    /*lo que sigue contiene todos los casos que no son borde*/
    else {
        iter->act = iter->act->prox;
        if (iter->act == iter->ant->prox) {
            iter->ant = iter->act;
        } else {
            iter->ant->prox = iter->act;
        }
        free(aux->clave);
        free(aux);
        iter->lista->tam--;
        return dato;
    }

}
