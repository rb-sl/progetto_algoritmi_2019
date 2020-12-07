/* ------ ----- ------- ------
 *  
 * Politecnico di Milano - Scuola di Ingegneria Industriale e dell'Informazione
 * Corso di Ingegneria Informatica
 * Prova finale di Algoritmi e Principi dell'Informatica (Prof. Mandrioli)
 * A.A. 2018/2019
 * 
 * Sviluppo di un sistema di gestione di entita' e relazioni
 * v6.4.1.1
 * Uso una tabella di hash per le entita' (cui vengono collegati RB-tree) e RB-tree per la lista 
 * delle relazioni e per quella delle sorgenti (sotto entrel). Gli alberi di entrel (connessioni
 * tra entita' e relazioni) e src sono anche inseriti in una lista, per permettere efficienza
 * sia per i report che per le insert.
 * 
 * Convenzioni:
 * 
 * S => costo spaziale
 * T => costo temporale
 * Calcolati nel caso peggiore senza eventuali funzioni interne
 * 
 * n => lunghezza dell'input
 * h => numero di elementi nella tabella delle entita'
 * r => numero di elementi nell'albero di relazioni
 * e => numero di elementi negli alberi di entrel, in totale O(r*h)
 * s => numero di elementi negli alberi di sorgenti, in totale O(r*h^2)
 * 
 * Con <*> si intendono gli algoritmi tratti dal libro, eventualmente riadattati
 * 
 */ 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* /----------------------\
 * | DEFINIZIONE COSTANTI |
 * \----------------------/
 */

/* Colore rosso - usato anche come true */
#define RED 'R'
/* Colore nero - usato anche come false */
#define BLACK 'B'
/* Marcatore di cella rossa in discesa */
#define REDDOWN 'S'
/* Marcatore di cella nera in discesa */
#define BLACKDOWN 'C'
/* Marcatore di cella rossa in salita */
#define REDUP 'Q'
/* Marcatore di cella nera in salita */
#define BLACKUP 'A'
/* Lunghezza massima dei comandi */
#define COMDIM 7
/* Lunghezza base di ogni parametro */
#define PARSIZE 50
/* Numero di valori della stringa considerati dalla funzione di hash */
#define HASHVAL 7
/* Numero di celle della tabella di hash */
#define HASHDIM 251

/* -----------------------
 * | DEFINIZIONE COMANDI |
 * -----------------------
 */

/* Comando di aggiunta entita' */
#define C_ADDENT "addent"
/* Comando di cancellazione entita' */
#define C_DELENT "delent"
/* Comando di aggiunta relazione */
#define C_ADDREL "addrel"
/* Comando di cancellazione relazione */
#define C_DELREL "delrel"
/* Comando di stampa */
#define C_REPORT "report"
/* Comando di fine */
#define C_END "end"

/* /-----------------------\
 * | DEFINIZIONE STRUTTURE |
 * \-----------------------/
 */

/* Struct per la gestione delle entita'; elemento di una tabella hash gestita con rb-tree 
 * S = 41B + strlen(id_ent) = O(n)
 */
typedef struct _ent
{
    /* Id dell'entita' */
    char* id_ent;
    /* Lista delle relazioni di cui e' destinazione l'entita' */
    struct _er* head_entrel;
    /* Lista delle sorgenti che si riferiscono all'entita' */
    struct _src* head_src;

    /* Figlio sx */
    struct _ent* left;
    /* Figlio dx */
    struct _ent* right;
    /* Genitore */
    struct _ent* parent;
    /* Colore */
    char color;
} entity;

/* Struct per le relazioni, elemento di un rb-tree 
 * S = 50B + strlen(id_rel) = O(n)
 */
typedef struct _rel
{
    /* Chiave - id della relazione */
    char* id_rel;

    /* Numero massimo di sorgenti */
    int nmax;
    /* Numero di entrel aventi nmax sorgenti */
    int maxcnt;
    /* Indicatore di validita' della maxlist 
     * BLACK => non valido 
     * RED => valido
     */
    char val;
    /* Lista dei massimi - cache */
    struct _msr* maxlist;
    /* Albero di entrel */
    struct _er* root_er;

    /* Figlio sx delle relazioni */
    struct _rel* left;
    /* Figlio dx delle relazioni */
    struct _rel* right;
    /* Genitore */
    struct _rel* parent;
    /* Colore */
    char color;
} relation;

/* Struct che associa ad ogni entita' l'rb-tree delle sue sorgenti per una relazione 
 * S = 69B = O(1)
 */
typedef struct _er
{
    /* Relazione cui si riferisce l'entrel */
    struct _rel* rel;
    /* Destinazione chui si riferisce */
    struct _ent* dst;
    /* Radice dell'albero delle sorgenti */
    struct _src* root_src;
    /* Contatore delle sorgenti */
    int nsrc;

    /* Struttura a partire da rel */

    /* Figlio sx per rel */
    struct _er* left;
    /* Figlio dx per rel */
    struct _er* right;
    /* Genitore per rel */
    struct _er* parent;
    /* Colore per rel */
    char color;

    /* Struttura a partire da una dst */

    /* Elemento precedente */
    struct _er* prev;
    /* Elemento successivo */
    struct _er* next;
} entrel;

/* Struct per la lista di sorgenti, parte di un rb-tree 
 * S = 57B = O(1)
 */
typedef struct _src
{
    /* Chiave - puntatore all'entità sorgente */
    struct _ent* src;
    /* Entrel cui si riferisce */
    struct _er* origin;

    /* Struttura da entrel */

    /* Figlio sx */
    struct _src* left;
    /* Figlio dx */
    struct _src* right;
    /* Genitore */
    struct _src* parent;
    /* Colore */
    char color;

    /* Struttura da una src */

    /* Elemento precedente */
    struct _src* prev;
    /* Elemento successivo */
    struct _src *next;
} source;

/* Struct per la lista di massimi, utilizzata per le cache dei report
 * S = 16B = O(1)
*/
typedef struct _msr
{
    struct _ent* dst;
    struct _msr* next;
} maxsrc;

/* /----------\
 * | FUNZIONI |
 * \----------/
 */

/* Funzione per la lettura di un parametro */
void read_param(char[]);

/* --------------------------
 * | FUNZIONI DA SPECIFICHE |
 * --------------------------
 */

/* Funzione per l'aggiunta di una entita' */
void addent();
/* Funzione per la cancellazione di una entita' */
void delent();
/* Funzione per l'aggiunta di una relazione */
void addrel();
/* Funzione per la cancellazione di una relazione */
void delrel();
/* Funzione per il report */
void report();

/* --------------------------------------------------
 * | FUNZIONI PER LA TABELLA DI HASH DELLE ENTITIES | 
 * --------------------------------------------------
 */

/* Funzione di hash */
int hashent(char*);

/* Funzione di ricerca di un elemento nella tabella, ne ritorna l'indirizzo */
entity* tab_search(char*);

/* RB-transplant per entity */
void ent_tplant(int,entity*,entity*);

/* Left rotate per entity */
void l_ent_rotate(int,entity*);

/* Right rotate per entity */
void r_ent_rotate(int,entity*);

/* ----------------------------------------
 * | FUNZIONI PER L'ALBERO DELLE RELATION |
 * ----------------------------------------
 */

/* Funzione che inserisce una relazione +fixup, ritorna il suo indirizzo */
relation* rel_insert(char*);

/* Ricerca e cancellazione di una relation +fixup */
void rel_delete(relation*);

/* RB-transplant per relation */
void rel_tplant(relation*,relation*);

/* Left rotate per relation */
void l_rel_rotate(relation*);

/* Right rotate per relation */
void r_rel_rotate(relation*);

/* --------------------------------------
 * | FUNZIONI PER L'ALBERO DELLE ENTREL |
 * --------------------------------------
 */

/* Ricerca e inserimento di una entrel +fixup, ne ritorna il puntatore */
entrel* entrel_insert(entity*,relation*);

/* Ricerca e cancellazione di una entrel +fixup */
void entrel_delete(entrel*);

/* RB-transplant per entrel da rel, ritorna la radice */
entrel* entrel_tplant(entrel*,entrel*,entrel*);

/* Left rotate per entrel da rel, ritorna la radice */
entrel* l_entrel_rotate(entrel*,entrel*);

/* Right rotate per entrel da rel, ritorna la radice */
entrel* r_entrel_rotate(entrel*,entrel*);

/* --------------------------------------
 * | FUNZIONI PER L'ALBERO DELLE SOURCE |
 * --------------------------------------
 */

/* Funzione per ricerca/inserimento di source +fixup */
void src_insert(entrel*,entity*,entity*);

/* Ricerca/cancellazione di una source +fixup */
void src_delete(source*);

/* RB-transplant per source, ritorna la radice */
source* src_tplant(source*,source*,source*);

/* Left rotate per source, ritorna la radice */
source* l_src_rotate(source*,source*);

/* Right rotate per source, ritorna la radice */
source* r_src_rotate(source*,source*);

/* Unset a partire da una entrel */
void src_unset(source*);

/* -----------------------------------
 * | FUNZIONI PER LA LISTA DI MAXSRC |
 * -----------------------------------
 */

void max_insert(maxsrc**,maxsrc**,entrel*);

void max_unset(maxsrc*);

/* /-------------------\
 * | VARIABILI GLOBALI |
 * \-------------------/
 */

/* Tabella di hash contenente le entità e i rispettivi alberi di sorgenti */
entity* entities[HASHDIM];

/* Albero contenente le relazioni */
relation* relations;

/* Elemento T.NIL per relation */
relation* rNIL;
/* Elemento T.NIL per entrel */
entrel* eNIL;
/* Elemento T.NIL per source */
source* sNIL;
/* T.NIL per hash di ent */
entity* hNIL;

int main()
{
    /* Stringa in cui viene salvato il comando letto */
    char command[COMDIM+1];
    /* Contatore */
    int i;
    /* Carattere buffer per lettura */
    char c;

    /*** INIZIALIZZAZIONI ***/
    hNIL=malloc(sizeof(entity));
    rNIL=malloc(sizeof(relation));
    eNIL=malloc(sizeof(entrel));
    sNIL=malloc(sizeof(source));

    /* Inizializzazione hNIL */
    hNIL->id_ent=NULL;
    hNIL->head_entrel=eNIL;
    hNIL->head_src=sNIL;

    hNIL->left=hNIL;
    hNIL->right=hNIL;
    hNIL->parent=hNIL;
    hNIL->color=BLACK;

    /* Inizializzazione rNIL */
    rNIL->id_rel=NULL;

    rNIL->nmax=-1;
    rNIL->maxcnt=0;
    rNIL->val=BLACK;
    rNIL->maxlist=NULL;
    rNIL->root_er=eNIL;

    rNIL->left=rNIL;
    rNIL->right=rNIL;
    rNIL->parent=rNIL;
    rNIL->color=BLACK;

    /* Inizializzazione eNIL */
    eNIL->rel=rNIL;
    eNIL->dst=hNIL;
    eNIL->root_src=sNIL;
    eNIL->nsrc=0;

    eNIL->left=eNIL;
    eNIL->right=eNIL;
    eNIL->parent=eNIL;
    eNIL->color=BLACK;

    eNIL->prev=eNIL;
    eNIL->next=eNIL;

    /* Inizializzazione sNIL */
    sNIL->src=hNIL;
    sNIL->origin=eNIL;

    sNIL->left=sNIL;
    sNIL->right=sNIL;
    sNIL->parent=sNIL;
    sNIL->color=BLACK; 

    sNIL->prev=sNIL;
    sNIL->next=sNIL;

    /* Inizializzazione albero delle relazioni */
    relations=rNIL;

    /* Inizializzazione tabella di hash delle entita' */
    for(i=0;i<HASHDIM;i++)
        entities[i]=hNIL;

    /*** INIZIO CORPO DELL'ALGORITMO ***/

    /* Lettura fuori ciclo */
    i=0;
    c=getchar();
    while(c!=EOF && c!=' ' && c!='\n')
    {
        command[i]=c;
        i++;
        c=getchar();
    }
    command[i]='\0';

    while(strncmp(command,C_END,3)!=0) 
    {
        /* Selezione delle operazioni da svolgere a seconda del comando ricevuto */
        if(strcasecmp(command,C_ADDENT)==0)
            addent();
        else if(strcasecmp(command,C_DELENT)==0) 
            delent();
        else if(strcasecmp(command,C_ADDREL)==0)
            addrel();
        else if(strcasecmp(command,C_DELREL)==0) 
            delrel();   
        else if(strcasecmp(command,C_REPORT)==0)
            report();

        /* Lettura per reiniziare operazioni */
        while(c!='\n')
            c=getchar();
        c=getchar();
        i=0;
        while(c!=EOF && c!=' ' && c!='\n')
        {
            command[i]=c;
            i++;
            c=getchar();
        }
        command[i]='\0';
    }

    return 0;
}

/* /----------\
 * | FUNZIONI |
 * \----------/
 */

/* La funzione legge e salva nel parametro (passato per riferimento)
 * una stringa letta tra " "
 * T = O(n)
 */
void read_param(char par[])
{
    /* Carattere buffer */
    char c;
    /* Contatore caratteri letti e indice in par*/
    int i;

    c=0;
    while(c!='\"')
        c=getchar();

    i=0;
    c=getchar();
    while(c!=EOF && c!='\"')
    {
        par[i]=c;
        i++;
        c=getchar();
    }
    par[i]='\0';

    return;
}

/* --------------------------
 * | FUNZIONI DA SPECIFICHE |
 * --------------------------
 */

/* Funzione che inizializza e aggiunge nella tabella
 * di hash o in un rb-tree una nuova entita'
 * T = O(log(h))
 */
void addent()
{
    /* Nome dell'entita' */
    char id_ent[PARSIZE];
    /* Valore dell'hash di id_ent */
    int h;
    /* Valore per la comparazione */
    int cmp;
    /* Nuovo elemento della tabella delle entità */
    entity* new;
    /* Buffer per ricerca */
    entity* y;
    entity* x;
    /* Buffer per fixup */
    entity* tofix;

    read_param(id_ent);

    h=hashent(id_ent);

    /* <RB-INSERT> per entity */
    y=hNIL;
    x=entities[h];
    cmp=-1;
    while(x!=hNIL)
    {
        y=x;
        cmp=strcmp(id_ent,x->id_ent);
        if(cmp==0)
            return;
        else if(cmp<0)
            x=x->left;
        else
            x=x->right;
    }

    new=malloc(sizeof(entity));

    /* Inizializzazione new */
    new->id_ent=strdup(id_ent);
    new->head_entrel=eNIL;
    new->head_src=sNIL;
    
    new->left=hNIL;
    new->right=hNIL;
    new->parent=y;
    if(y==hNIL)
        entities[h]=new; 
    else if(strcmp(id_ent,y->id_ent)<0)
        y->left=new;
    else
        y->right=new;
    new->color=RED;

    /* <RB-INSERT-FIXUP> per entities */
    tofix=new;

    while((tofix->parent)->color==RED)
        if(tofix->parent==((tofix->parent)->parent)->left)
        {
            y=((tofix->parent)->parent)->right;
            if(y->color==RED)
            {
                (tofix->parent)->color=BLACK;
                y->color=BLACK;
                ((tofix->parent)->parent)->color=RED;
                tofix=(tofix->parent)->parent;
            }
            else 
            {
                if(tofix==(tofix->parent)->right)
                {
                    tofix=tofix->parent;

                    l_ent_rotate(h,tofix);
                }
                (tofix->parent)->color=BLACK;
                ((tofix->parent)->parent)->color=RED;

                r_ent_rotate(h,(tofix->parent)->parent);
            }
        }
        else
        {
            y=((tofix->parent)->parent)->left;
            if(y->color==RED)
            {
                (tofix->parent)->color=BLACK;
                y->color=BLACK;
                ((tofix->parent)->parent)->color=RED;
                tofix=(tofix->parent)->parent;
            }
            else 
            {
                if(tofix==(tofix->parent)->left)
                {
                    tofix=tofix->parent;
                   
                    r_ent_rotate(h,tofix);
                }
                (tofix->parent)->color=BLACK;
                ((tofix->parent)->parent)->color=RED;
                
                l_ent_rotate(h,(tofix->parent)->parent);
            }
        }
    entities[h]->color=BLACK;
    /* Fine <RB-INSERT-FIXUP> */

    return;
}

/* Funzione che rimuove una entita' dalla tabella
 * T = O(log(h)+e+s)
 */
void delent()
{
    /* Nome dell'entita' */
    char id_ent[PARSIZE];
    /* Entita' da cancellare */
    entity* todel;
    /* Buffer per ricerca */
    entity* x;
    entity* y;
    /* Colore originale di y - per fixup */
    char og_y;
    /* Valore di hash */
    int h;
    /* Buffer per fixup */
    entity* w;
    /* Buffer per l'unset delle src */
    source* s_head;
    source* s_next;
    /* Buffer per l'unset delle entrel */
    entrel* e_head;
    entrel* e_next;

    read_param(id_ent);

    /* Controllo dell'esistenza dell'elemento */
    todel=tab_search(id_ent);

    if(todel)
    {
        h=hashent(id_ent);
        /* <RB-DELETE> per ent */
        y=todel;
        og_y=y->color;

        if(todel->left==hNIL)
        {
            x=todel->right;
            ent_tplant(h,todel,todel->right);
        }
        else if(todel->right==hNIL)
        {
            x=todel->left;
            ent_tplant(h,todel,todel->left);
        }
        else
        {
            /* <TREE-MINIMUM(todel->right)> */
            y=todel->right;
            while(y->left!=hNIL)
                y=y->left;
            /* Fine tree-minimum */
            og_y=y->color;
            x=y->right;
            if(y->parent==todel)
                x->parent=y;
            else
            {
                ent_tplant(h,y,y->right);
                y->right=todel->right;
                (y->right)->parent=y;
            }
            ent_tplant(h,todel,y);
            y->left=todel->left;
            (y->left)->parent=y;
            y->color=todel->color;            
        }
        if(og_y==BLACK)
        {
            /* <RB-DELETE-FIXUP> per entity */
            while(x!=entities[h] && x->color==BLACK)
                if(x==x->parent->left)
                {
                    w=x->parent->right;
                    if(w->color==RED)
                    {
                        w->color=BLACK;
                        x->parent->color=RED;
                        l_ent_rotate(h,x->parent);
                        w=x->parent->right;
                    }
                    if((w->left)->color==BLACK && (w->right)->color==BLACK)
                    {
                        w->color=RED;
                        x=x->parent;
                    }
                    else 
                    {
                        if((w->right)->color==BLACK)
                        {
                            w->left->color=BLACK;
                            w->color=RED;
                            r_ent_rotate(h,w);
                            w=(x->parent)->right;
                        }
                        w->color=(x->parent)->color;
                        x->parent->color=BLACK;
                        w->right->color=BLACK;
                        l_ent_rotate(h,x->parent); 
                        x=entities[h];
                    }
                }
                else
                {
                    w=x->parent->left;
                    if(w->color==RED)
                    {
                        w->color=BLACK;
                        x->parent->color=RED;
                        r_ent_rotate(h,x->parent);
                        w=x->parent->left;
                    }
                    if((w->right)->color==BLACK && (w->left)->color==BLACK)
                    {
                        w->color=RED;
                        x=x->parent;
                    }
                    else 
                    {
                        if((w->left)->color==BLACK)
                        {
                            w->right->color=BLACK;
                            w->color=RED;
                            l_ent_rotate(h,w);
                            w=(x->parent)->left;
                        }
                        w->color=(x->parent)->color;
                        x->parent->color=BLACK;
                        w->left->color=BLACK;
                        r_ent_rotate(h,x->parent); 
                        x=entities[h];
                    }
                }
            x->color=BLACK; 
            /* Fine <RB-DELETE-FIXUP> */
        }

        /* Unset delle src relative all'entita' */
        s_head=(todel->head_src);
        while(s_head!=sNIL)
        {
            s_next=s_head->next;
            src_delete(s_head);
            s_head=s_next;
        }

        /* Unset delle entrel dell'entita' */
        e_head=todel->head_entrel;
        while(e_head!=eNIL)
        {
            e_next=e_head->next;
            entrel_delete(e_head);
            e_head=e_next;
        }

        /* Cancellazione id_ent ed elemento */
        free(todel->id_ent);
        free(todel);
    }

    return;
}

/* Funzione per l'aggiunta di una relazione dopo
 * un controllo sull'esistenza
 * T = O(log(r))
 */
void addrel()
{
    /* Parametri del comando */
    char id_src[PARSIZE];
    char id_dst[PARSIZE];
    char id_rel[PARSIZE];
    /* Puntatore all'id della relazione */
    relation* prel;
    /* Puntatore alla sorgente */
    entity* src;
    /* Puntatore alla destinazione */
    entity* dst;
    /* Puntatore alla relazione in entrel */
    entrel* er;

    read_param(id_src);    
    /* Cerca src e, se non esiste, si ferma */
    src=tab_search(id_src);
    if(!src)
        return;

    read_param(id_dst);
    /* Cerca dst e, se non esiste, si ferma*/
    dst=tab_search(id_dst);
    if(!dst)        
        return;

    read_param(id_rel);
    /* Inserimento sia nelle relazioni che nelle entrel di dst */
    prel=rel_insert(id_rel);

    er=entrel_insert(dst,prel);

    src_insert(er,dst,src);

    return;
}

/* Funzione per la cancellazione di una rel: dopo aver validato la
 * richiesta cancella la sorgente
 * T = O(log(r)+log(e)+log(s)) 
 */
void delrel()
{
    /* Parametri del comando */
    char id_src[PARSIZE];
    char id_dst[PARSIZE];
    char id_rel[PARSIZE];
    /* Puntatore all'id della entrel */
    entrel* er;
    /* Puntatore alla sorgente da cancellare */
    source* todel;
    /* Buffer per la comparazione */
    int cmp;
    /* Buffer per ricerca */
    source* x;
    relation* y;
    entrel* z;
    /* Puntatore alla relazione */
    relation* rel;

    read_param(id_src);
    read_param(id_dst);
    read_param(id_rel);

    /* Ricerca della relazione che punta a dst; se non esiste termina */
    rel=rNIL;
    y=relations;
    cmp=-1;
    while(y!=rNIL && cmp!=0)
    {
        rel=y;
        cmp=strcmp(id_rel,y->id_rel);
        if(cmp<0)
            y=y->left;
        else if(cmp>0)
            y=y->right;
    }
    if(cmp!=0)
        return;

    /* Ricerca dell'entrel */
    er=eNIL;
    z=rel->root_er;
    cmp=-1;
    while(z!=eNIL && cmp!=0)
    {
        er=z;
        cmp=strcmp(id_dst,(z->dst)->id_ent);
        if(cmp<0)
            z=z->left;
        else if(cmp>0)
            z=z->right;
    }
    if(cmp!=0)    
        return;
    
    /* Ricerca della sorgente da eliminare; se non esiste termina */
    todel=sNIL;
    x=er->root_src;
    cmp=-1;
    while(x!=sNIL && cmp!=0)
    {
        todel=x;
        cmp=strcmp(id_src,(x->src)->id_ent);
        if(cmp<0)
            x=x->left;
        else if(cmp>0)
            x=x->right;
    }
    if(cmp!=0)
        return;
        
    src_delete(todel);

    return;
}

/* Funzione per mostrare il report come da specifiche. 
 * T = O(r*e)
 */
void report(void)
{
    /* Buffer per gli alberi di entrel */
    entrel* er;
    /* Buffer per gli alberi relation */
    relation* rel;
    /* Contatore per il numero di entrel con nmax */
    int cnt;
    /* nmax temporaneo */
    int tmax;
    /* Puntatore per maxlist */
    maxsrc* curr;
    maxsrc* prev;

    rel=relations;
    
    /* Se non esistono relazioni stampa none e ritorna */
    if(rel==rNIL)
    {
        printf("none\n");
        return;
    }

    /* Per ogni relation */
    while(rel!=rNIL)
    {
        /* Se la cella non e' marcata, la marca in discesa e scende */
        if(rel->color==RED || rel->color==BLACK)
            /* Scende fino all'elemento piu' piccolo marcando in dicesa le celle toccate, R=>S , B=>C */
            while(rel->left!=rNIL)
            {
                rel->color++;
                rel=rel->left;
            }
        
        /* Se la cella e' gia' marcata in salita la smarca e passa al padre, Q=>R , A=>B */
        if(rel->color==REDUP || rel->color==BLACKUP)
        {
            rel->color++;
            rel=rel->parent;
        }
        else
        {
            /* Altrimenti effettua il corpo codice */

            /* Stampa del nome della relazione */
            fputs("\"",stdout);
            fputs(rel->id_rel,stdout);
            fputs("\" ",stdout);
            
            if(rel->val==RED)
            {
                /* Caso 1: maxlist valida 
                * Stampa la maxlist senza modificare nulla
                */
                curr=rel->maxlist;
                while(curr)
                {
                    fputs("\"",stdout);
                    fputs((curr->dst)->id_ent,stdout);
                    fputs("\" ",stdout);

                    curr=curr->next;
                }
            }
            else
            {
                /* Caso 4: maxcnt==-1
                 * Implica nmax non valido, pertanto bisogna anche cercarlo.
                 * Costruisce prima la lista, poi la stampa
                 */
                tmax=0;
                cnt=0;
                er=rel->root_er;
                curr=rel->maxlist;
                prev=NULL;
                while(er!=eNIL)
                {
                    if(er->color==RED || er->color==BLACK)
                        while(er->left!=eNIL)
                        {
                            er->color++;
                            er=er->left;
                        }

                    if(er->color==REDUP || er->color==BLACKUP)
                    {
                        er->color++;
                        er=er->parent;
                    }
                    else
                    {
                        if(er->nsrc>tmax)
                        {
                            tmax=er->nsrc;
                            cnt=1;
                            curr=rel->maxlist;
                            prev=NULL;
                            
                            max_insert(&curr,&prev,er);                           
                        }
                        else if (er->nsrc==tmax)
                        {
                            max_insert(&curr,&prev,er);
                            cnt++;
                        }

                        if((er->color==REDDOWN || er->color==BLACKDOWN || er->left==eNIL) && er->right!=eNIL)
                        {
                            if(er->color!=RED && er->color!=BLACK)
                                er->color-=2;
                            else
                                er->color--;

                            er=er->right;   
                        }
                        else
                        {
                            if(er->color!=RED && er->color!=BLACK)
                                er->color--;
                            er=er->parent;
                        }
                    }
                }
                /* Cancellazione dei rimanenti elementi in maxlist */
                if(curr && prev && curr!=prev)
                {
                    max_unset(curr);
                    if(prev)
                        prev->next=NULL;
                }
                /* Aggiornamento di nmax */
                rel->nmax=tmax;
                /* Aggiornamento di maxcnt */
                rel->maxcnt=cnt;
                /* Abilitazione della cache */
                rel->val=RED;

                /* Stampa entita' */
                curr=rel->maxlist;
                while(curr)
                {
                    fputs("\"",stdout);
                    fputs((curr->dst)->id_ent,stdout);
                    fputs("\" ",stdout);

                    curr=curr->next;
                }
            }
            /* Stampa del numero */
            printf("%d; ",rel->nmax);
    
            /* Fine corpo */

            /* Dopo le operazioni, se la cella era marcata in discesa e esiste un altro elemento */
            if((rel->color==REDDOWN || rel->color==BLACKDOWN || rel->left==rNIL) && rel->right!=rNIL)
            {
                /* Marca in salita rel e passa al successivo */
                if(rel->color!=RED && rel->color!=BLACK)
                    rel->color-=2;
                else
                    rel->color--;

                rel=rel->right;   
            }
            else
            {
                /* Altrimenti, se necessario, smarca rel e risale */
                if(rel->color!=RED && rel->color!=BLACK)
                    rel->color--;
                rel=rel->parent;
            } 
        }
    }
    printf("\n");    

    return;
}

/* --------------------------------------------------
 * | FUNZIONI PER LA TABELLA DI HASH DELLE ENTITIES |
 * --------------------------------------------------
 */

/* La funzione di hash ritorna la somma dei primi HASHVAL 
 * caratteri (visti come interi) modulo HASHDIM 
 * T = O(1)
 */
int hashent(char* str)
{
    /* Indice di id_ent */
    int i;
    /* Valore di hash */
    int h;

    h=0;
    i=0;
    while(i<HASHVAL && str[i]!='\0')
    {   
        h+=str[i];
        i++;
    }

    return h%HASHDIM;
}

/* Funzione che ricerca nella tabella tab l'entita' id_ent
 * e ne ritorna l'indirizzo o NULL se non esiste
 * T Peggiore = O(h), medio = O(1)
 */
entity* tab_search(char* id_ent)
{
    /* Puntatore per scorrere la tabella */
    entity* p;
    entity* x;
    entity* y;
    int cmp;

    /* Calcolo dell'hash della stringa e posizionamento nella tabella */
    p=entities[hashent(id_ent)];

    y=hNIL;
    x=p;
    while(x!=hNIL)
    {
        y=x;
        cmp=strcmp(id_ent,x->id_ent);
        if(cmp==0)
            return y;
        else if(cmp<0)
            x=x->left;
        else
            x=x->right;
    }

    return NULL;
}

/* <RB-TRANSPLANT> per entity
 * T = O(1)
 */
void ent_tplant(int hash,entity* u,entity* v)
{
    if(u->parent==hNIL)
		entities[hash]=v;
	else if(u==(u->parent)->left)
		(u->parent)->left=v;
	else
		(u->parent)->right=v;

	v->parent=u->parent;

	return;
}

/* <LEFT-ROTATE> per entity
 * T = O(1)
 */
void l_ent_rotate(int hash,entity* tor)
{
    /* Buffer per rotate */
    entity* w;

    w=tor->right;
    tor->right=w->left;

    if(w->left!=hNIL)
        (w->left)->parent=tor;

    w->parent=tor->parent;

    if(tor->parent==hNIL)
        entities[hash]=w;
    else if(tor==(tor->parent)->left)
        (tor->parent)->left=w;
    else
        (tor->parent)->right=w;
    
    w->left=tor;
    tor->parent=w;

    return;
}

/* <RIGHT-ROTATE> per entity
 * T = O(1)
 */
void r_ent_rotate(int hash,entity* tor)
{
    /* Buffer per rotate */
    entity* w;

    w=tor->left;
    tor->left=w->right;

    if(w->right!=hNIL)
        (w->right)->parent=tor;

    w->parent=tor->parent;

    if(tor->parent==hNIL)
        entities[hash]=w;
    else if(tor==(tor->parent)->right)
        (tor->parent)->right=w;
    else
        (tor->parent)->left=w;
    
    w->right=tor;
    tor->parent=w;

    return;
}

/* ----------------------------------------
 * | FUNZIONI PER L'ALBERO DELLE RELATION |
 * ----------------------------------------
 */

/* Funzione per l'inserimento di un elemento nell'albero di relazioni,
 * solo se non e' presente, con fixup
 * T = O(log(r))
 */
relation* rel_insert(char* id_rel)
{
    /* Nuova relazione da inserire */
    relation* new; 
    /* Risultato della strcasecmp */
    int cmp;
    /* Buffer per la insert */
    relation* x;
    relation* y;
    /* Buffer per la fixup */
    relation* tofix;
    
    /* <RB-INSERT> per relation */
    y=rNIL;
    x=relations;
    while(x!=rNIL)
    {
        y=x;
        cmp=strcmp(id_rel,x->id_rel);
        if(cmp==0)
            return y;
        else if(cmp<0)
            x=x->left;
        else
            x=x->right;
    }
    
    new=malloc(sizeof(relation));

    /* Inizializzazione rel */
    new->id_rel=strdup(id_rel);
    new->nmax=0;
    new->maxcnt=0;
    new->val=BLACK;
    new->maxlist=NULL;
    new->root_er=eNIL;

    new->left=rNIL;
    new->right=rNIL;
    new->parent=y;
    if(y==rNIL)
        relations=new; 
    else if(strcmp(id_rel,y->id_rel)<0)
        y->left=new;
    else
        y->right=new;
    new->color=RED;   
    /* Fine <RB-INSERT> */

    /* <RB-INSERT-FIXUP> per relations */
    tofix=new;

    while((tofix->parent)->color==RED)
        if(tofix->parent==((tofix->parent)->parent)->left)
        {
            y=((tofix->parent)->parent)->right;
            if(y->color==RED)
            {
                (tofix->parent)->color=BLACK;
                y->color=BLACK;
                ((tofix->parent)->parent)->color=RED;
                tofix=(tofix->parent)->parent;
            }
            else 
            {
                if(tofix==(tofix->parent)->right)
                {
                    tofix=tofix->parent;

                    l_rel_rotate(tofix);
                }
                (tofix->parent)->color=BLACK;
                ((tofix->parent)->parent)->color=RED;

                r_rel_rotate((tofix->parent)->parent);
            }
        }
        else
        {
            y=((tofix->parent)->parent)->left;
            if(y->color==RED)
            {
                (tofix->parent)->color=BLACK;
                y->color=BLACK;
                ((tofix->parent)->parent)->color=RED;
                tofix=(tofix->parent)->parent;
            }
            else 
            {
                if(tofix==(tofix->parent)->left)
                {
                    tofix=tofix->parent;
                   
                    r_rel_rotate(tofix);
                }
                (tofix->parent)->color=BLACK;
                ((tofix->parent)->parent)->color=RED;
                
                l_rel_rotate((tofix->parent)->parent);
            }
        }
    relations->color=BLACK;
    /* Fine <RB-INSERT-FIXUP> */

    return new; 
}

/* Funzione per la cancellazione di una relazione + fixup
 * T = O(log(r))
 */
void rel_delete(relation* todel)
{
    /* Buffer per la cancellazione */
    relation* x;
    relation* y;
    /* Colore originale di y */
    char og_y;
    /* Buffer per fixup */
	relation* w;

	/* <RB-DELETE> per rel */
    y=todel;
    og_y=y->color;

    if(todel->left==rNIL)
    {
        x=todel->right;
        rel_tplant(todel,todel->right);
    }
    else if(todel->right==rNIL)
    {
        x=todel->left;
        rel_tplant(todel,todel->left);
    }
    else
    {
        /* <TREE-MINIMUM(todel->right)> */
        y=todel->right;
        while(y->left!=rNIL)
            y=y->left;
        /* Fine tree-minimum */
        og_y=y->color;
        x=y->right;
        if(y->parent==todel)
            x->parent=y;
        else
        {
            rel_tplant(y,y->right);
            y->right=todel->right;
            (y->right)->parent=y;
        }
        rel_tplant(todel,y);
        y->left=todel->left;
        (y->left)->parent=y;
        y->color=todel->color;            
    }
    if(og_y==BLACK)
    {
        /* <RB-DELETE-FIXUP> per source */
        while(x!=relations && x->color==BLACK)
            if(x==x->parent->left)
            {
                w=x->parent->right;
                if(w->color==RED)
                {
                    w->color=BLACK;
                    x->parent->color=RED;
                    l_rel_rotate(x->parent);
                    w=x->parent->right;
                }
                if((w->left)->color==BLACK && (w->right)->color==BLACK)
                {
                    w->color=RED;
                    x=x->parent;
                }
                else 
                {
                    if((w->right)->color==BLACK)
                    {
                        w->left->color=BLACK;
                        w->color=RED;
                        r_rel_rotate(w);
                        w=(x->parent)->right;
                    }
                    w->color=(x->parent)->color;
                    x->parent->color=BLACK;
                    w->right->color=BLACK;
                    l_rel_rotate(x->parent); 
                    x=relations;
                }
            }
            else
            {
                w=x->parent->left;
                if(w->color==RED)
                {
                    w->color=BLACK;
                    x->parent->color=RED;
                    r_rel_rotate(x->parent);
                    w=x->parent->left;
                }
                if((w->right)->color==BLACK && (w->left)->color==BLACK)
                {
                    w->color=RED;
                    x=x->parent;
                }
                else 
                {
                    if((w->left)->color==BLACK)
                    {
                        w->right->color=BLACK;
                        w->color=RED;
                        l_rel_rotate(w);
                        w=(x->parent)->left;
                    }
                    w->color=(x->parent)->color;
                    x->parent->color=BLACK;
                    w->left->color=BLACK;
                    r_rel_rotate(x->parent); 
                    x=relations;
                }
            }
	    x->color=BLACK; 
        /* Fine <RB-DELETE-FIXUP> */
    }

    free(todel->id_rel);
    max_unset(todel->maxlist);
    free(todel);
    
	return;
}

/* <RB-TRANSPLANT> per relation
 * T = O(1)
 */
void rel_tplant(relation* u,relation* v)
{
    if(u->parent==rNIL)
		relations=v;
	else if(u==(u->parent)->left)
		(u->parent)->left=v;
	else
		(u->parent)->right=v;

	v->parent=u->parent;

	return;
}

/* <LEFT-ROTATE> per relation
 * T = O(1)
 */
void l_rel_rotate(relation* tor)
{
    /* Buffer per rotate */
    relation* w;

    w=tor->right;
    tor->right=w->left;

    if(w->left!=rNIL)
        (w->left)->parent=tor;

    w->parent=tor->parent;

    if(tor->parent==rNIL)
        relations=w;
    else if(tor==(tor->parent)->left)
        (tor->parent)->left=w;
    else
        (tor->parent)->right=w;
    
    w->left=tor;
    tor->parent=w;

    return;
}

/* <RIGHT-ROTATE> per relation
 * T = O(1)
 */
void r_rel_rotate(relation* tor)
{
    /* Buffer per rotate */
    relation* w;

    w=tor->left;
    tor->left=w->right;

    if(w->right!=rNIL)
        (w->right)->parent=tor;

    w->parent=tor->parent;

    if(tor->parent==rNIL)
        relations=w;
    else if(tor==(tor->parent)->right)
        (tor->parent)->right=w;
    else
        (tor->parent)->left=w;
    
    w->right=tor;
    tor->parent=w;

    return;
}

/* --------------------------------------
 * | FUNZIONI PER L'ALBERO DELLE ENTREL |
 * --------------------------------------
 */

/* Funzione per l'inserimento di una entrel nell'albero da dst, solo se
 * la combinazione non e' gia' presente
 * T = O(log(e))
 */
entrel* entrel_insert(entity* dst,relation* rel)
{
    /* Buffer per la ricerca dell'entrel */
    entrel* x;
    entrel* y;
    /* Buffer per la comparazione */
    int cmp;
    /* Elemento da inserire */
    entrel* new;
    /* Buffer per la fixup */
    entrel* tofix;

    /* <RB-INSERT> per entrel */
    y=eNIL;
    x=rel->root_er;
    cmp=-1;
    while(x!=eNIL && cmp!=0)
    {
        y=x;
        cmp=strcmp(dst->id_ent,(x->dst)->id_ent);
        if(cmp==0)
            return y;
        else if(cmp<0)
            x=x->left;
        else if(cmp>0)
            x=x->right;
    }

    new=malloc(sizeof(entrel));

    /* Inizializzazione new */
    new->rel=rel;
    new->dst=dst;
    new->root_src=sNIL;
    new->nsrc=0;

    new->left=eNIL;
    new->right=eNIL;
    new->parent=y;
    if(y==eNIL)
        rel->root_er=new;
    else if(strcmp(dst->id_ent,(y->dst)->id_ent)<0)
        y->left=new;
    else
        y->right=new;
    new->color=RED;

    /* Inserimento nella coda per dst */
    if(dst->head_entrel!=eNIL)
        (dst->head_entrel)->prev=new;
    new->next=dst->head_entrel;
    dst->head_entrel=new;
    new->prev=eNIL;

    /* Fine <RB-INSERT> */

    /* <RB-INSERT-FIXUP> per entrel */ 
    tofix=new;
    while((tofix->parent)->color==RED)
        if(tofix->parent==((tofix->parent)->parent)->left)
        {
            y=((tofix->parent)->parent)->right;
            if(y->color==RED)
            {
                (tofix->parent)->color=BLACK;
                y->color=BLACK;
                ((tofix->parent)->parent)->color=RED;
                tofix=(tofix->parent)->parent;
            }
            else 
            {
                if(tofix==(tofix->parent)->right)
                {
                    tofix=tofix->parent;

                    rel->root_er=l_entrel_rotate(rel->root_er,tofix);
                }
                (tofix->parent)->color=BLACK;
                ((tofix->parent)->parent)->color=RED;

                rel->root_er=r_entrel_rotate(rel->root_er,(tofix->parent)->parent);
            }
        }
        else
        {
            y=((tofix->parent)->parent)->left;
            if(y->color==RED)
            {
                (tofix->parent)->color=BLACK;
                y->color=BLACK;
                ((tofix->parent)->parent)->color=RED;
                tofix=(tofix->parent)->parent;
            }
            else 
            {
                if(tofix==(tofix->parent)->left)
                {
                    tofix=tofix->parent;
                    rel->root_er=r_entrel_rotate(rel->root_er,tofix);
                }
                (tofix->parent)->color=BLACK;
                ((tofix->parent)->parent)->color=RED;
                
                rel->root_er=l_entrel_rotate(rel->root_er,(tofix->parent)->parent);
            }
        }

    (rel->root_er)->color=BLACK;
    /* Fine <RB-INSERT-FIXUP> */

    return new;
}

/* Funzione per la cancellazione di una entrel, avviene in caso di delent o se non
 * esiste nessuna src nel suo albero
 * T = O(log(e))
 */
void entrel_delete(entrel* todel)
{
    /* Buffer per la cancellazione */
    entrel* x;
    entrel* y;
    /* Radice delle entrel */
    entrel* root;
    /* Colore originale di y */
    char og_y;
    /* Buffer per fixup */
	entrel* w;

    root=(todel->rel)->root_er;

    /* <RB-DELETE> per entrel */
    y=todel;
    og_y=y->color;

    if(todel->left==eNIL)
    {
        x=todel->right;
        root=entrel_tplant(root,todel,todel->right);
    }
    else if(todel->right==eNIL)
    {
        x=todel->left;
        root=entrel_tplant(root,todel,todel->left);
    }
    else
    {
        /* <TREE-MINIMUM(todel->right)> */
        y=todel->right;
        while(y->left!=eNIL)
            y=y->left;
        /* Fine tree-minimum */

        og_y=y->color;
        x=y->right;
        if(y->parent==todel)
            x->parent=y;
        else
        {
            root=entrel_tplant(root,y,y->right);
            y->right=todel->right;
            (y->right)->parent=y;
        }
        root=entrel_tplant(root,todel,y);
        y->left=todel->left;
        (y->left)->parent=y;
        y->color=todel->color;            
    }
    if(og_y==BLACK)
    {
        /* <RB-DELETE-FIXUP> per entrel */
        while(x!=root && x->color==BLACK)
            if(x==(x->parent)->left)
            {
                w=(x->parent)->right;
                if(w->color==RED)
                {
                    w->color=BLACK;
                    (x->parent)->color=RED;
                    root=l_entrel_rotate(root,x->parent);
                    w=(x->parent)->right;
                }
                if((w->left)->color==BLACK && (w->right)->color==BLACK)
                {
                    w->color=RED;
                    x=x->parent;
                }
                else 
                {
                    if((w->right)->color==BLACK)
                    {
                        (w->left)->color=BLACK;
                        w->color=RED;
                        root=r_entrel_rotate(root,w);
                        w=(x->parent)->right;
                    }
                    w->color=(x->parent)->color;
                    x->parent->color=BLACK;
                    w->right->color=BLACK;
                    root=l_entrel_rotate(root,x->parent); 
                    x=root;
                }
            }
            else
            {
                w=(x->parent)->left;
                if(w->color==RED)
                {
                    w->color=BLACK;
                    x->parent->color=RED;
                    root=r_entrel_rotate(root,x->parent);
                    w=(x->parent)->left;
                }
                if((w->right)->color==BLACK && (w->left)->color==BLACK)
                {
                    w->color=RED;
                    x=x->parent;
                }
                else 
                {
                    if((w->left)->color==BLACK)
                    {
                        w->right->color=BLACK;
                        w->color=RED;
                        root=l_entrel_rotate(root,w);
                        w=(x->parent)->left;
                    }
                    w->color=(x->parent)->color;
                    x->parent->color=BLACK;
                    w->left->color=BLACK;
                    root=r_entrel_rotate(root,x->parent); 
                    x=root;
                }
            }
	    x->color=BLACK; 
        /* Fine <RB-DELETE-FIXUP> per rel */
    }
    (todel->rel)->root_er=root;

    /* Delete per la lista di dst */
    if(todel->prev!=eNIL)
        (todel->prev)->next=todel->next;
    else
        (todel->dst)->head_entrel=todel->next;
    if(todel->next!=eNIL)
        (todel->next)->prev=todel->prev;
    
    /* Cancellazione di tutte le sorgenti della entrel */ 
    if(todel->root_src!=sNIL)
        src_unset(todel->root_src);

    if((todel->rel)->root_er==eNIL)
        rel_delete((todel->rel));
    else
        if((todel->rel)->nmax==todel->nsrc && (todel->rel)->maxcnt!=-1)
        {
            (todel->rel)->maxcnt--;
            if((todel->rel)->maxcnt==0)
                (todel->rel)->maxcnt=-1;
            (todel->rel)->val=BLACK;
        }

    free(todel);
    
	return;
}

/* <RB-TRANSPLANT> per entrel da dst
 * T = O(1)
 */
entrel* entrel_tplant(entrel* root,entrel* u,entrel* v)
{
	if(u->parent==eNIL)
		root=v;
	else if(u==(u->parent)->left)
		(u->parent)->left=v;
	else
		(u->parent)->right=v;

	v->parent=u->parent;

	return root;
}

/* <LEFT-ROTATE> entrel su dst
 * T = O(1)
 */
entrel* l_entrel_rotate(entrel* root,entrel* tor)
{
    /* Buffer per rotate */
    entrel* w;

    w=tor->right;
    tor->right=w->left;

    if(w->left!=eNIL)
        (w->left)->parent=tor;

    w->parent=tor->parent;

    if(tor->parent==eNIL)
        root=w;
    else if(tor==(tor->parent)->left)
        (tor->parent)->left=w;
    else
        (tor->parent)->right=w;
    
    w->left=tor;
    tor->parent=w;

    return root;
}

/* <RIGHT ROTATE> entrel per dst
 * T = O(1)
 */
entrel* r_entrel_rotate(entrel* root,entrel* tor)
{
    /* Buffer per rotate */
    entrel* w;

    w=tor->left;
    tor->left=w->right;

    if(w->right!=eNIL)
        (w->right)->parent=tor;

    w->parent=tor->parent;

    if(tor->parent==eNIL)
        root=w;
    else if(tor==(tor->parent)->right)
        (tor->parent)->right=w;
    else
        (tor->parent)->left=w;
    
    w->right=tor;
    tor->parent=w;

    return root;
}

/* --------------------------------------
 * | FUNZIONI PER L'ALBERO DELLE SOURCE |
 * --------------------------------------
 */

/* Funzione di inserimento di una source data la radice da entrel, 
 * dopo un controllo sull'esistenza. Fa anche la fixup
 * T = O(log(s))
 */
void src_insert(entrel* origin,entity* dst,entity* src)
{
    /* Buffer per la ricerca della radice dell'albero delle sorgenti */
    source* x;
    source* y;
    /* Buffer per la comparazione */
    int cmp;
    /* Elemento da inserire */
    source* new;
    /* Buffer per fixup */
    source* tofix;
    /* Buffer per entrel */
    source* root_er;

    root_er=origin->root_src;

    /* <RB-INSERT> per source da entrel */
    y=sNIL;
    x=origin->root_src;
    while(x!=sNIL)
    {
        y=x;
        cmp=strcmp(src->id_ent,(x->src)->id_ent);
        if(cmp==0)
            return;
        else if(cmp<0)
            x=x->left;
        else
            x=x->right;
    }

    new=malloc(sizeof(source));

    /* Inizializzazione new */
    new->src=src;
    new->origin=origin;

    new->left=sNIL;
    new->right=sNIL;
    new->parent=y;
    if(y==sNIL)
        root_er=new; 
    else if(strcmp(src->id_ent,(y->src)->id_ent)<0)
        y->left=new;
    else
        y->right=new;
    new->color=RED;

    /* Inserimento in lista di dst_src */
    if(src->head_src!=sNIL)
        (src->head_src)->prev=new;
    new->next=src->head_src;
    src->head_src=new;
    new->prev=sNIL;

    /* Fine <RB-INSERT> */

    /* <RB-INSERT-FIXUP> per source da entrel */
    tofix=new;
    while((tofix->parent)->color==RED)
        if(tofix->parent==((tofix->parent)->parent)->left)
        {
            y=((tofix->parent)->parent)->right;
            if(y->color==RED)
            {
                (tofix->parent)->color=BLACK;
                y->color=BLACK;
                ((tofix->parent)->parent)->color=RED;
                tofix=(tofix->parent)->parent;
            }
            else 
            {
                if(tofix==(tofix->parent)->right)
                {
                    tofix=tofix->parent;
                    root_er=l_src_rotate(root_er,tofix);
                }
                (tofix->parent)->color=BLACK;
                ((tofix->parent)->parent)->color=RED;

                root_er=r_src_rotate(root_er,(tofix->parent)->parent);
            }
        }
        else
        {
            y=((tofix->parent)->parent)->left;
            if(y->color==RED)
            {
                (tofix->parent)->color=BLACK;
                y->color=BLACK;
                ((tofix->parent)->parent)->color=RED;
                tofix=(tofix->parent)->parent;
            }
            else 
            {
                if(tofix==(tofix->parent)->left)
                {
                    tofix=tofix->parent;
                   
                    root_er=r_src_rotate(root_er,tofix);
                }
                (tofix->parent)->color=BLACK;
                ((tofix->parent)->parent)->color=RED;
                
                root_er=l_src_rotate(root_er,(tofix->parent)->parent);
            }
        }

    root_er->color=BLACK;
    /* Fine <RB-INSERT-FIXUP> */ 
    
    origin->nsrc++;

    /* Cambio di nmax */
    if((origin->rel)->nmax<origin->nsrc)
    {
        (origin->rel)->nmax=origin->nsrc;
        (origin->rel)->maxcnt=1;
        (origin->rel)->val=BLACK;
    }
    else if((origin->rel)->nmax==origin->nsrc)
    {
        (origin->rel)->maxcnt++;
        (origin->rel)->val=BLACK;
    }

    origin->root_src=root_er;

    return;
}

/* <RB-DELETE> per source, include fixup
 * Se l'albero della sua entrel e' vuoto procede a cancellarla
 * T = O(1)
 */
void src_delete(source* todel)
{
    /* Relazione cui si riferisce todel */
    relation* rel;
    /* Origine di todel */
    entrel* er;
    /* Buffer per la cancellazione */
    source* x;
    source* y;
    /* Radice delle sorgenti */
    source* root;
    /* Colore originale di y */
    char og_y;
    /* Buffer per fixup */
	source* w;
    
    er=todel->origin;    
    root=er->root_src;
    /* <RB-DELETE> per source  da entrel */
    y=todel;
    og_y=y->color;

    if(todel->left==sNIL)
    {
        x=todel->right;
        root=src_tplant(root,todel,todel->right);
    }
    else if(todel->right==sNIL)
    {
        x=todel->left;
        root=src_tplant(root,todel,todel->left);
    }
    else
    {
        /* <TREE-MINIMUM(todel->right)> */
        y=todel->right;
        while(y->left!=sNIL)
            y=y->left;
        /* Fine <TREE_MINIMUM> */
        og_y=y->color;
        x=y->right;
        if(y->parent==todel)
            x->parent=y;
        else
        {
            root=src_tplant(root,y,y->right);
            y->right=todel->right;
            (y->right)->parent=y;
        }
        root=src_tplant(root,todel,y);
        y->left=todel->left;
        (y->left)->parent=y;
        y->color=todel->color;            
    }
    if(og_y==BLACK)
    {
        /* <RB-DELETE-FIXUP> per src su entrel */
        while(x!=root && x->color==BLACK)
            if(x==(x->parent)->left)
            {
                w=(x->parent)->right;
                if(w->color==RED)
                {
                    w->color=BLACK;
                    (x->parent)->color=RED;
                    root=l_src_rotate(root,x->parent);
                    w=(x->parent)->right;
                }
                if((w->left)->color==BLACK && (w->right)->color==BLACK)
                {
                    w->color=RED;
                    x=x->parent;
                }
                else 
                {
                    if((w->right)->color==BLACK)
                    {
                        (w->left)->color=BLACK;
                        w->color=RED;
                        root=r_src_rotate(root,w);
                        w=(x->parent)->right;
                    }
                    w->color=(x->parent)->color;
                    x->parent->color=BLACK;
                    w->right->color=BLACK;
                    root=l_src_rotate(root,x->parent); 
                    x=root;
                }
            }
            else
            {
                w=(x->parent)->left;
                if(w->color==RED)
                {
                    w->color=BLACK;
                    x->parent->color=RED;
                    root=r_src_rotate(root,x->parent);
                    w=(x->parent)->left;
                }
                if((w->right)->color==BLACK && (w->left)->color==BLACK)
                {
                    w->color=RED;
                    x=x->parent;
                }
                else 
                {
                    if((w->left)->color==BLACK)
                    {
                        w->right->color=BLACK;
                        w->color=RED;
                        root=l_src_rotate(root,w);
                        w=(x->parent)->left;
                    }
                    w->color=(x->parent)->color;
                    (x->parent)->color=BLACK;
                    (w->left)->color=BLACK;
                    root=r_src_rotate(root,x->parent); 
                    x=root;
                }
            }
            x->color=BLACK;
    }
    /* Fine <RB-DELETE-FIXUP> */
    er->root_src=root;

    rel=er->rel;

    /* Controllo per la cancellazione di entrel */
    if(er->root_src==sNIL)
        entrel_delete(er);
    else
    {
        if(rel->nmax==er->nsrc)
        {
            rel->maxcnt--;
            if(rel->maxcnt==0)
                rel->nmax--;
            rel->val=BLACK;
        }
        er->nsrc--;
    }

    /* Delete per la lista da ent */
    if(todel->prev!=sNIL)
        (todel->prev)->next=todel->next;
    else
        (todel->src)->head_src=todel->next;
    if(todel->next!=sNIL)
        (todel->next)->prev=todel->prev;

    free(todel);

	return;
}

/* <RB-TRANSPLANT> per source, ritorna la radice 
 * T = O(1)
 */
source* src_tplant(source* root,source* u,source* v)
{
	if(u->parent==sNIL)
		root=v;
	else if(u==(u->parent)->left)
		(u->parent)->left=v;
	else
		(u->parent)->right=v;

	v->parent=u->parent;

	return root;
}

/* <LEFT-ROTATE> per source
 * T = O(1)
 */
source* l_src_rotate(source* root,source* tor)
{
    /* Buffer per rotate */
    source* w;

    w=tor->right;
    tor->right=w->left;

    if(w->left!=sNIL)
        (w->left)->parent=tor;

    w->parent=tor->parent;

    if(tor->parent==sNIL)
        root=w;
    else if(tor==(tor->parent)->left)
        (tor->parent)->left=w;
    else
        (tor->parent)->right=w;
    
    w->left=tor;
    tor->parent=w;

    return root;
}

/* <RIGHT-ROTATE> per source
 * T = O(1)
 */
source* r_src_rotate(source* root,source* tor)
{
    /* Buffer per rotate */
    source* w;

    w=tor->left;
    tor->left=w->right;

    if(w->right!=sNIL)
        (w->right)->parent=tor;

    w->parent=tor->parent;

    if(tor->parent==sNIL)
        root=w;
    else if(tor==(tor->parent)->right)
        (tor->parent)->right=w;
    else
        (tor->parent)->left=w;
    
    w->right=tor;
    tor->parent=w;

    return root;
}

/* Funzione per l'unset di un albero di source a partire da una entrel
 * T = O(s) 
 */
void src_unset(source* todel)
{
    /* Buffer per cancellazione */
    source* next;
    
    while(todel!=sNIL)
    {
        /* Scende fino all'elemento piu' a sinistra di todel poi, a destra */
        while(todel->left!=sNIL || todel->right!=sNIL)
            if(todel->left!=sNIL)
                todel=todel->left;
            else
                todel=todel->right;
                
        /* Salva il padre dell'elemento da cancellare */
        next=todel->parent;

        /* Cancella il puntatore del padre a seconda che todel si trovi a sx o dx */
        if(todel==next->left)
            next->left=sNIL;
        else
            next->right=sNIL;

        /* Elimina l'elemento dalla lista di src */
        if(todel->prev!=sNIL)
            (todel->prev)->next=todel->next;
        else
            (todel->src)->head_src=todel->next;
        if(todel->next!=sNIL)
            (todel->next)->prev=todel->prev;

        /* Cancellazione fisica */
        free(todel);

        /* Se esiste un sottoalbero destro lo dealloca con la stessa tattica (minimo, risalgo) */
        if(next->right!=sNIL)
            todel=next->right;
        else /* altrimenti risale */
            todel=next;
    }
    
    return;
}

/* -----------------------------------
 * | FUNZIONI PER LA LISTA DI MAXSRC |
 * -----------------------------------
 */

/* Funzione per l'inserimento nella maxlist data la entrel
 * Chiamata solo all'interno del report
 * T = O(1) 
 */
void max_insert(maxsrc** curr,maxsrc** prev,entrel* er)
{
    /* Se non esiste prev non esiste nemmeno curr */
    if(!*prev && !*curr)
    {
        *prev=malloc(sizeof(maxsrc));
        (*prev)->next=NULL;
        (*prev)->dst=er->dst;

        (er->rel)->maxlist=*prev;
    }        
    else if(!*curr)
    {
        (*curr)=malloc(sizeof(maxsrc));
        (*curr)->next=NULL;
        (*curr)->dst=er->dst;

        (*prev)->next=(*curr);
        
        *prev=*curr;
        *curr=NULL;
    }
    else
    {
        (*curr)->dst=er->dst;
        *prev=*curr;
        *curr=(*curr)->next;
    }
    return;
}

/* Funzione per la cancellazione di una lista di maxsrc
 * Chiamata solo da report
 * T = O(h)
 */
void max_unset(maxsrc* head)
{
    /* Buffer per cancellazione */
    maxsrc* next;

    while(head)
    {
        next=head->next;
        free(head);
        head=next;
    }

    return;
}