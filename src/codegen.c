/*
 * codegen.c - Geracao do analisador sintatico (etapa 5).
 *
 * Estrutura do arquivo .c gerado:
 *
 *   1. includes e conjuntos constantes (conj_0, conj_1, ...)
 *   2. estado e primitivas: avancar, match, erro_sintatico, arvore
 *   3. modo proprio: analisador lexico gerado a partir de TOKENS
 *      modo Micro C: tokens obtidos de proximo_token() (scanner do TP1)
 *   4. leitura de um arquivo de tokens (opcao --tokens do analisador)
 *   5. um procedimento parse_X() por nao terminal
 *   6. main
 *
 * Fornecido: toda a estrutura acima, os nomes dos tokens, os conjuntos,
 * as primitivas, o lexico (exceto o reconhecimento das expressoes
 * regulares) e a traducao de terminais e nao terminais.
 *
 * TODO(aluno):
 *   - gen_expr: traducao de sequencia, alternativa, [ ] e { }
 *     (esquema da secao 4 do enunciado);
 *   - gen_casamento_regex: converter uma expressao regular de TOKENS em
 *     codigo C (automato finito) que reconhece o maior prefixo;
 *   - gen_token_de_arquivo: leitura do arquivo de tokens do TP1.
 */
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "microc_mapa.h"
#include "util.h"

/* ------------------------------------------------------------------ */
/* Emissao de texto                                                   */
/* ------------------------------------------------------------------ */

typedef struct {
    FILE *f;
    int nivel;          /* nivel de indentacao (4 espacos por nivel) */
} Emissor;

/* Emite uma linha indentada; fmt segue printf. */
static void emit(Emissor *em, const char *fmt, ...)
{
    int i;
    va_list ap;
    for (i = 0; i < em->nivel; i++)
        fputs("    ", em->f);
    va_start(ap, fmt);
    vfprintf(em->f, fmt, ap);
    va_end(ap);
    fputc('\n', em->f);
}

/* Emite linhas fixas, sem interpretar '%'. A lista termina em NULL. */
static void emit_linhas(FILE *f, const char *const *linhas)
{
    for (; *linhas != NULL; linhas++) {
        fputs(*linhas, f);
        fputc('\n', f);
    }
}

/* Texto dentro de comentario C: evita fechar o comentario por engano. */
static void emitir_comentario(FILE *f, const char *s)
{
    for (; *s != '\0'; s++) {
        if (s[0] == '*' && s[1] == '/') {
            fputs("* /", f);
            s++;
        } else if (*s == '\n') {
            fputc(' ', f);
        } else {
            fputc(*s, f);
        }
    }
}

/* String C entre aspas, com escapes. */
static void emitir_string_c(FILE *f, const char *s)
{
    fputc('"', f);
    for (; *s != '\0'; s++) {
        unsigned char c = (unsigned char) *s;
        if (c == '"' || c == '\\')
            fprintf(f, "\\%c", c);
        else if (c == '\n')
            fputs("\\n", f);
        else if (c == '\t')
            fputs("\\t", f);
        else if (c < 32 || c > 126)
            fprintf(f, "\\%03o", c);
        else
            fputc(c, f);
    }
    fputc('"', f);
}

/* ------------------------------------------------------------------ */
/* Nomes dos tokens no codigo gerado                                  */
/* ------------------------------------------------------------------ */

static const char *const nomes_simbolos[][2] = {
    { "+", "MAIS" }, { "-", "MENOS" }, { "*", "VEZES" }, { "/", "BARRA" },
    { "%", "PORCENTO" }, { "(", "ABRE_PAR" }, { ")", "FECHA_PAR" },
    { "[", "ABRE_COL" }, { "]", "FECHA_COL" }, { "{", "ABRE_CHAVE" },
    { "}", "FECHA_CHAVE" }, { ";", "PV" }, { ",", "VIRGULA" },
    { ".", "PONTO" }, { ":", "DOIS_PONTOS" }, { "=", "IGUAL" },
    { "==", "IGUAL_IGUAL" }, { "!=", "DIFERENTE" }, { "<", "MENOR" },
    { ">", "MAIOR" }, { "<=", "MENOR_IGUAL" }, { ">=", "MAIOR_IGUAL" },
    { "&&", "E_LOGICO" }, { "||", "OU_LOGICO" }, { "!", "NAO" },
    { ":=", "ATRIBUICAO" }, { "->", "SETA" }
};

static int eh_palavra(const char *s)
{
    if (!isalpha((unsigned char) *s) && *s != '_')
        return 0;
    for (; *s != '\0'; s++)
        if (!isalnum((unsigned char) *s) && *s != '_')
            return 0;
    return 1;
}

static int nome_em_uso(const Gramatica *g, int ate, const char *nome)
{
    int i;
    for (i = 0; i < ate; i++)
        if (strcmp(g->terms[i].nome_c, nome) == 0)
            return 1;
    return 0;
}

/* Define g->terms[i].nome_c para todos os terminais. */
static void nomear_terminais(Gramatica *g, int microc)
{
    int i;
    size_t k;
    char buf[256];
    Terminal *t;

    for (i = 0; i < g->nterms; i++) {
        t = &g->terms[i];
        free(t->nome_c);
        if (i == TERM_FIM) {
            t->nome_c = xstrdup(microc ? "END_OF_FILE" : "T_FIM");
            continue;
        }
        if (microc) {
            t->nome_c = xstrdup(t->literal ? microc_nome_literal(t->nome) : t->nome);
            continue;
        }
        buf[0] = '\0';
        if (!t->literal) {
            sprintf(buf, "T_%.200s", t->nome);
        } else if (eh_palavra(t->nome) && strlen(t->nome) < 200) {
            sprintf(buf, "T_%s", t->nome);
            for (k = 2; buf[k] != '\0'; k++)
                buf[k] = (char) toupper((unsigned char) buf[k]);
        } else {
            for (k = 0; k < sizeof(nomes_simbolos) / sizeof(nomes_simbolos[0]); k++)
                if (strcmp(nomes_simbolos[k][0], t->nome) == 0)
                    sprintf(buf, "T_%s", nomes_simbolos[k][1]);
            if (buf[0] == '\0')
                sprintf(buf, "T_LIT_%d", i);
        }
        if (nome_em_uso(g, i, buf))
            sprintf(buf + strlen(buf), "_%d", i);
        t->nome_c = xstrdup(buf);
    }
}

/* ------------------------------------------------------------------ */
/* Conjuntos constantes                                               */
/* ------------------------------------------------------------------ */

#define MAX_COMPARACOES 3   /* ate 3 tokens: tok.tipo == A || ...; acima: pertence */

static Bitset *conjuntos = NULL;
static int nconj = 0, capconj = 0;

static int bitset_igual(const Bitset *a, const Bitset *b)
{
    int i;
    for (i = 0; i < a->n; i++)
        if (bitset_tem(a, i) != bitset_tem(b, i))
            return 0;
    return 1;
}

/* Registra um conjunto e devolve seu numero N (emitido como conj_N).
 * Conjuntos iguais compartilham a mesma constante. */
static int registrar_conjunto(const Bitset *b)
{
    int i;
    for (i = 0; i < nconj; i++)
        if (bitset_igual(&conjuntos[i], b))
            return i;
    if (nconj == capconj) {
        capconj = capconj ? 2 * capconj : 16;
        conjuntos = (Bitset *) xrealloc(conjuntos, (size_t) capconj * sizeof(Bitset));
    }
    bitset_iniciar(&conjuntos[nconj], b->n);
    bitset_copiar(&conjuntos[nconj], b);
    return nconj++;
}

static void liberar_conjuntos(void)
{
    int i;
    for (i = 0; i < nconj; i++)
        bitset_liberar(&conjuntos[i]);
    free(conjuntos);
    conjuntos = NULL;
    nconj = capconj = 0;
}

static void emitir_conjuntos(FILE *f, const Gramatica *g)
{
    int i, t;
    for (i = 0; i < nconj; i++) {
        fprintf(f, "static const int conj_%d[] = { ", i);
        for (t = bitset_proximo(&conjuntos[i], 0); t >= 0;
             t = bitset_proximo(&conjuntos[i], t + 1))
            fprintf(f, "%s, ", g->terms[t].nome_c);
        fputs("-1 };\n", f);
    }
}

/* Escreve a condicao "o token corrente pertence a b":
 *   tok.tipo == T_A                     (um token)
 *   tok.tipo == T_A || tok.tipo == T_B  (ate MAX_COMPARACOES)
 *   pertence(tok.tipo, conj_N)          (mais tokens) */
static void emitir_teste(FILE *f, const Gramatica *g, const Bitset *b)
{
    int t, n = bitset_tamanho(b), primeiro = 1;
    if (n == 0) {
        fputs("0", f);
    } else if (n <= MAX_COMPARACOES) {
        for (t = bitset_proximo(b, 0); t >= 0; t = bitset_proximo(b, t + 1)) {
            fprintf(f, "%stok.tipo == %s", primeiro ? "" : " || ", g->terms[t].nome_c);
            primeiro = 0;
        }
    } else {
        fprintf(f, "pertence(tok.tipo, conj_%d)", registrar_conjunto(b));
    }
}

/* Linha "<antes><teste><depois>", ex.: emit_teste(em, "while (", g, b, ") {") */
static void emit_teste(Emissor *em, const char *antes, const Gramatica *g,
                       const Bitset *b, const char *depois)
{
    int i;
    for (i = 0; i < em->nivel; i++)
        fputs("    ", em->f);
    fputs(antes, em->f);
    emitir_teste(em->f, g, b);
    fputs(depois, em->f);
    fputc('\n', em->f);
}

/* Linha "erro_sintatico(conj_N);" com N = conjunto dos tokens esperados. */
static void emit_erro(Emissor *em, const Bitset *esperados)
{
    emit(em, "erro_sintatico(conj_%d);", registrar_conjunto(esperados));
}

/* ------------------------------------------------------------------ */
/* Traducao das expressoes (esquema da secao 4 do enunciado)          */
/* ------------------------------------------------------------------ */

static void gen_expr(Emissor *em, const Gramatica *g, const Expr *e)
{
    int i, primeiro;
    const Expr *anulavel;

    switch (e->tipo) {
    case E_TERM:
        emit(em, "match(%s);", g->terms[e->simbolo].nome_c);
        break;
    case E_NAOTERM:
        emit(em, "parse_%s();", g->nts[e->simbolo].nome);
        break;
    case E_SEQ:
        for (i = 0; i < e->nfilhos; i++)
            gen_expr(em, g, e->filhos[i]);
        break;
    case E_ALT:
        primeiro = 1;
        anulavel = NULL;
        for (i = 0; i < e->nfilhos; i++) {
            const Expr *a = e->filhos[i];
            if (a->anulavel) {
                anulavel = a;   /* vai para o else final */
                continue;
            }
            emit_teste(em, primeiro ? "if (" : "} else if (", g, &a->first, ") {");
            em->nivel++;
            gen_expr(em, g, a);
            em->nivel--;
            primeiro = 0;
        }
        if (anulavel != NULL && anulavel->tipo == E_VAZIO) {
            emit(em, "}");      /* alternativa vazia: nada a fazer no else */
        } else {
            emit(em, "} else {");
            em->nivel++;
            if (anulavel != NULL)
                gen_expr(em, g, anulavel);
            else
                emit_erro(em, &e->first);
            em->nivel--;
            emit(em, "}");
        }
        break;
    case E_OPC:
    case E_REP:
        emit_teste(em, e->tipo == E_OPC ? "if (" : "while (", g,
                   &e->filhos[0]->first, ") {");
        em->nivel++;
        gen_expr(em, g, e->filhos[0]);
        em->nivel--;
        emit(em, "}");
        break;
    case E_VAZIO:
        break;
    }
}

/* ------------------------------------------------------------------ */
/* Expressoes regulares de TOKENS (modo proprio)                      */
/* ------------------------------------------------------------------ */

/* funcoes auxiliares de gen_casamento_regex . 
 * Caminho: expressao -> AFN de Thompson -> AFD pela construcao
 * de subconjuntos -> switch em C.
 *
 * Expressoes aceitas :
 *     alt    = concat { "|" concat }
 *     concat = { repet }
 *     repet  = atomo { "*" | "+" | "?" }
 *     atomo  = "(" alt ")" | "[" ["^"] faixas "]" | "." | "\" c | c
 */

/* Estado do AFN: OU uma aresta por um conjunto de caracteres (classe),
 * OU ate duas arestas vazias. -1 significa "sem aresta". */
typedef struct {
    int por_classe;
    int prox1, prox2;
    unsigned char classe[256];   /* classe[c] = 1 se c segue a aresta */
} EstadoAfn;

typedef struct {
    EstadoAfn *v;     /* estados do AFN */
    int n;
    const char *re;   /* expressao sendo lida */
    size_t pos;       /* posicao atual em re */
    int erro;         /* 1 se a expressao e invalida */
} Afn;

/* Pedaco do AFN, com um estado inicial e um final. */
typedef struct {
    int ini, fim;
} Frag;

static int novo_estado(Afn *a)
{
    EstadoAfn *e = &a->v[a->n];

    memset(e, 0, sizeof *e);
    e->prox1 = e->prox2 = -1;
    return a->n++;
}

static void ligar_vazio(Afn *a, int de, int para)
{
    if (a->v[de].prox1 < 0)
        a->v[de].prox1 = para;
    else
        a->v[de].prox2 = para;
}

/* Fragmento que casa um caractere da classe cl. */
static Frag frag_classe(Afn *a, const unsigned char *cl)
{
    Frag f;

    f.ini = novo_estado(a);
    f.fim = novo_estado(a);
    a->v[f.ini].por_classe = 1;
    a->v[f.ini].prox1 = f.fim;
    memcpy(a->v[f.ini].classe, cl, 256);
    return f;
}

/* Le um caractere. Escapes: \n, \t e \r; nos demais (\\, \/, \. ...)
 * vale o proprio caractere. */
static int ler_char(Afn *a)
{
    int c = (unsigned char) a->re[a->pos];

    if (c == '\\') {
        c = (unsigned char) a->re[++a->pos];
        if (c == '\0') {
            a->erro = 1;   /* barra no fim da expressao */
            return 0;
        }
        if (c == 'n')
            c = '\n';
        else if (c == 't')
            c = '\t';
        else if (c == 'r')
            c = '\r';
    }
    a->pos++;
    return c;
}

/* Classe "[...]", com o "[" ja lido: caracteres, faixas a-z e [^...].
 * Um '-' no fim da classe e literal, como em [+-]. */
static Frag parse_classe(Afn *a)
{
    unsigned char cl[256];
    int negada = 0, lo, hi, c;

    memset(cl, 0, sizeof cl);
    if (a->re[a->pos] == '^') {
        negada = 1;
        a->pos++;
    }
    while (!a->erro && a->re[a->pos] != ']') {
        if (a->re[a->pos] == '\0') {
            a->erro = 1;   /* falta ']' */
            break;
        }
        lo = hi = ler_char(a);
        if (a->re[a->pos] == '-' && a->re[a->pos + 1] != ']'
            && a->re[a->pos + 1] != '\0') {
            a->pos++;
            hi = ler_char(a);
        }
        for (c = lo; c <= hi; c++)
            cl[c] = 1;
    }
    if (!a->erro)
        a->pos++;   /* ']' */
    if (negada)
        for (c = 0; c < 256; c++)
            cl[c] = (unsigned char) !cl[c];
    return frag_classe(a, cl);
}

static Frag parse_alt(Afn *a);

static Frag parse_atomo(Afn *a)
{
    unsigned char cl[256];
    Frag f;
    int c = (unsigned char) a->re[a->pos];

    memset(cl, 0, sizeof cl);
    if (c == '(') {
        a->pos++;
        f = parse_alt(a);
        if (a->re[a->pos] == ')')
            a->pos++;
        else
            a->erro = 1;   /* falta ')' */
        return f;
    }
    if (c == '[') {
        a->pos++;
        return parse_classe(a);
    }
    if (c == '*' || c == '+' || c == '?') {
        a->erro = 1;       /* operador sem operando */
        return frag_classe(a, cl);
    }
    if (c == '.') {        /* qualquer caractere, menos '\n' */
        a->pos++;
        for (c = 1; c < 256; c++)
            cl[c] = 1;
        cl['\n'] = 0;
    } else {
        cl[ler_char(a)] = 1;
    }
    return frag_classe(a, cl);
}

/* a*, a+ e a?: um estado novo de entrada (r.ini) e um de saida (r.fim). */
static Frag parse_repet(Afn *a)
{
    Frag f = parse_atomo(a), r;
    char op;

    while (!a->erro && (a->re[a->pos] == '*' || a->re[a->pos] == '+'
                        || a->re[a->pos] == '?')) {
        op = a->re[a->pos++];
        r.ini = novo_estado(a);
        r.fim = novo_estado(a);
        ligar_vazio(a, r.ini, f.ini);
        if (op != '+')                  /* * e ?: pode pular a */
            ligar_vazio(a, r.ini, r.fim);
        ligar_vazio(a, f.fim, r.fim);
        if (op != '?')                  /* * e +: pode repetir a */
            ligar_vazio(a, f.fim, f.ini);
        f = r;
    }
    return f;
}

/* ab: liga o final de a ao inicio de b. Vazia: um estado so. */
static Frag parse_concat(Afn *a)
{
    Frag f, g;
    char c = a->re[a->pos];

    if (c == '\0' || c == '|' || c == ')') {
        f.ini = f.fim = novo_estado(a);
        return f;
    }
    f = parse_repet(a);
    for (;;) {
        c = a->re[a->pos];
        if (a->erro || c == '\0' || c == '|' || c == ')')
            return f;
        g = parse_repet(a);
        ligar_vazio(a, f.fim, g.ini);
        f.fim = g.fim;
    }
}

/* a|b: um estado novo que se divide entre a e b, e um que os junta. */
static Frag parse_alt(Afn *a)
{
    Frag f = parse_concat(a), g, r;

    while (!a->erro && a->re[a->pos] == '|') {
        a->pos++;
        g = parse_concat(a);
        r.ini = novo_estado(a);
        r.fim = novo_estado(a);
        ligar_vazio(a, r.ini, f.ini);
        ligar_vazio(a, r.ini, g.ini);
        ligar_vazio(a, f.fim, r.fim);
        ligar_vazio(a, g.fim, r.fim);
        f = r;
    }
    return f;
}

/* AFD: cada estado e um conjunto de estados do AFN, guardado como um
 * vetor de marcas com nafn posicoes. */
typedef struct {
    unsigned char *conj;   /* conj[d * nafn + s] = 1 se s pertence a d */
    int *trans;            /* trans[d * 256 + c] = destino, ou -1 */
    int *final;            /* final[d] = 1 se d contem o final do AFN */
    int n, cap, nafn;
} Afd;

/* Acrescenta a b os estados alcancaveis por arestas vazias. */
static void fecho_vazio(const Afn *a, unsigned char *b, int *pilha)
{
    int topo = 0, s, t, k;

    for (s = 0; s < a->n; s++)
        if (b[s])
            pilha[topo++] = s;
    while (topo > 0) {
        s = pilha[--topo];
        if (a->v[s].por_classe)
            continue;
        for (k = 0; k < 2; k++) {
            t = k == 0 ? a->v[s].prox1 : a->v[s].prox2;
            if (t >= 0 && !b[t]) {
                b[t] = 1;
                pilha[topo++] = t;
            }
        }
    }
}

/* Numero do estado do AFD com o conjunto b, criado se ainda nao existe. */
static int estado_afd(Afd *d, const unsigned char *b, int final_afn)
{
    int i;
    size_t tam = (size_t) d->nafn;

    for (i = 0; i < d->n; i++)
        if (memcmp(d->conj + i * tam, b, tam) == 0)
            return i;
    if (d->n == d->cap) {
        d->cap = d->cap ? 2 * d->cap : 16;
        d->conj = (unsigned char *) xrealloc(d->conj, d->cap * tam);
        d->trans = (int *) xrealloc(d->trans, (size_t) d->cap * 256 * sizeof(int));
        d->final = (int *) xrealloc(d->final, (size_t) d->cap * sizeof(int));
    }
    memcpy(d->conj + d->n * tam, b, tam);
    for (i = 0; i < 256; i++)
        d->trans[d->n * 256 + i] = -1;
    d->final[d->n] = b[final_afn];
    return d->n++;
}

/* Construcao de subconjuntos. O laco em i alcanca os estados criados
 * durante ele. O caractere 0 nunca tem transicao: marca o fim da entrada. */
static void construir_afd(const Afn *a, Frag fr, Afd *d)
{
    unsigned char *b = (unsigned char *) xmalloc((size_t) a->n);
    int *pilha = (int *) xmalloc((size_t) a->n * sizeof(int));
    int i, c, s, algum;

    memset(d, 0, sizeof *d);
    d->nafn = a->n;
    memset(b, 0, (size_t) a->n);
    b[fr.ini] = 1;
    fecho_vazio(a, b, pilha);
    estado_afd(d, b, fr.fim);

    for (i = 0; i < d->n; i++) {
        for (c = 1; c < 256; c++) {
            memset(b, 0, (size_t) a->n);
            algum = 0;
            for (s = 0; s < a->n; s++)
                if (d->conj[(size_t) i * a->n + s] && a->v[s].por_classe
                    && a->v[s].classe[c]) {
                    b[a->v[s].prox1] = 1;
                    algum = 1;
                }
            if (algum) {
                fecho_vazio(a, b, pilha);
                d->trans[i * 256 + c] = estado_afd(d, b, fr.fim);
            }
        }
    }
    free(b);
    free(pilha);
}

/* Caractere como literal C legivel: 'a', '\n', ou o codigo numerico. */
static void emitir_char(FILE *f, int c)
{
    if (c == '\n')
        fputs("'\\n'", f);
    else if (c == '\t')
        fputs("'\\t'", f);
    else if (c == '\r')
        fputs("'\\r'", f);
    else if (c == '\'' || c == '\\')
        fprintf(f, "'\\%c'", c);
    else if (c >= 32 && c < 127)
        fprintf(f, "'%c'", c);
    else
        fprintf(f, "%d", c);
}

/* switch sobre o estado; caracteres seguidos com o mesmo destino viram
 * uma faixa (c >= x && c <= y). ultimo guarda o fim do maior prefixo. */
static void emitir_afd(FILE *f, const char *nome, const Afd *d)
{
    int i, c, fim, destino, primeiro;

    fprintf(f, "static size_t %s(const char *s)\n{\n", nome);
    fputs("    size_t i = 0, ultimo = 0;\n    int estado = 0, c;\n\n", f);
    fputs("    for (;;) {\n", f);
    fputs("        c = (unsigned char) s[i];\n", f);
    fputs("        switch (estado) {\n", f);
    for (i = 0; i < d->n; i++) {
        fprintf(f, "        case %d:\n", i);
        if (d->final[i])
            fputs("            ultimo = i;   /* estado final */\n", f);
        primeiro = 1;
        for (c = 1; c < 256; c = fim + 1) {
            destino = d->trans[i * 256 + c];
            fim = c;
            if (destino < 0)
                continue;
            while (fim + 1 < 256 && d->trans[i * 256 + fim + 1] == destino)
                fim++;
            fputs(primeiro ? "            if (" : "            else if (", f);
            if (fim == c) {
                fputs("c == ", f);
                emitir_char(f, c);
            } else {
                fputs("c >= ", f);
                emitir_char(f, c);
                fputs(" && c <= ", f);
                emitir_char(f, fim);
            }
            fprintf(f, ")\n                estado = %d;\n", destino);
            primeiro = 0;
        }
        if (primeiro)
            fputs("            return ultimo;\n", f);
        else
            fputs("            else\n                return ultimo;\n            break;\n", f);
    }
    fputs("        default:\n            return ultimo;\n", f);
    fputs("        }\n        i++;\n    }\n}\n\n", f);
}

/* Emite a funcao
 *     static size_t <nome>(const char *s)
 * que devolve o comprimento do MAIOR prefixo de s reconhecido pela
 * expressao regular, ou 0 se nenhum prefixo for reconhecido.
 * Retorna 0 se a expressao for valida (senao reporte com diag_erro). */
static int gen_casamento_regex(FILE *f, const char *nome, const char *regex)
{
    Afn a;
    Afd d;
    Frag fr;

    /* cada caractere da expressao cria no maximo 3 estados do AFN */
    a.v = (EstadoAfn *) xmalloc((3 * strlen(regex) + 2) * sizeof(EstadoAfn));
    a.n = 0;
    a.re = regex;
    a.pos = 0;
    a.erro = 0;
    fr = parse_alt(&a);
    if (a.erro || regex[a.pos] != '\0') {   /* ex.: ')' sobrando */
        diag_erro(0, 0, "expressao regular /%s/ invalida", regex);
        free(a.v);
        return -1;
    }
    construir_afd(&a, fr, &d);
    emitir_afd(f, nome, &d);
    free(d.conj);
    free(d.trans);
    free(d.final);
    free(a.v);
    return 0;
}


/* ------------------------------------------------------------------ */
/* Leitura do arquivo de tokens (opcao --tokens do analisador gerado) */
/* ------------------------------------------------------------------ */

/* Emite "static Token token_de_arquivo(void)", que le o proximo token
 * do arquivo aberto em arquivo_tokens, no formato gravado pelo lexer do
 * TP1 (livro-texto, secao 3.3.2):
 *
 *   Token: tipo = <numero>, lexema = '<texto>', linha = <numero>
 *
 * Ao chegar ao fim do arquivo devolve um token TOKEN_FIM. */
static void gen_token_de_arquivo(FILE *f, int microc)
{
    
    static const char *const inicio[] = {
        "static int linha_arquivo_tokens = 0;   /* linha do arquivo de tokens */",
        "static int ultima_linha_token = 1;     /* linha do ultimo token lido */",
        "",
        "/* Le o proximo token de arquivo_tokens, no formato",
        " *   Token: tipo = <numero>, lexema = '<texto>', linha = <numero>",
        " * No fim do arquivo devolve TOKEN_FIM. */",
        "static Token token_de_arquivo(void)",
        "{",
        "    char buf[1024];",
        "    const char *abre, *fecha, *p;",
        "    size_t n;",
        "    int tipo, lin;",
        "    Token t;",
        "",
        "    memset(&t, 0, sizeof t);",
        "    while (fgets(buf, sizeof buf, arquivo_tokens) != NULL) {",
        "        linha_arquivo_tokens++;",
        "        for (p = buf; *p == ' ' || *p == '\\t' || *p == '\\r' || *p == '\\n'; p++)",
        "            ;",
        "        if (*p == '\\0')",
        "            continue;",
        "        abre = strstr(buf, \"lexema = '\");",
        "        fecha = NULL;",
        "        for (p = strstr(buf, \"', linha =\"); p != NULL; p = strstr(p + 1, \"', linha =\"))",
        "            fecha = p;",
        "        if (sscanf(buf, \" Token: tipo = %d ,\", &tipo) != 1 || abre == NULL",
        "            || fecha == NULL || fecha < abre + 10",
        "            || sscanf(fecha, \"', linha = %d\", &lin) != 1) {",
        "            fprintf(stderr, \"arquivo de tokens, linha %d: linha malformada\\n\",",
        "                    linha_arquivo_tokens);",
        "            exit(EXIT_FAILURE);",
        "        }",
        "        abre += 10;",
        "        n = (size_t) (fecha - abre);",
        "        if (n >= sizeof t.lexema)",
        "            n = sizeof t.lexema - 1;",
        "        memcpy(t.lexema, abre, n);",
        "        t.lexema[n] = '\\0';",
        NULL
    };
    static const char *const fim[] = {
        "        t.linha = lin;",
        "        ultima_linha_token = lin;",
        "        return t;",
        "    }",
        "    t.tipo = TOKEN_FIM;",
        "    t.linha = ultima_linha_token;",
        "    return t;",
        "}",
        "",
        NULL
    };
    emit_linhas(f, inicio);
    /* no modo Micro C o tipo e TokenType; no modo proprio, int */
    fputs(microc ? "        t.tipo = (TokenType) tipo;\n" : "        t.tipo = tipo;\n", f);
    emit_linhas(f, fim);
}
/* ------------------------------------------------------------------ */
/* Partes fixas do codigo gerado                                      */
/* ------------------------------------------------------------------ */

static const char *const rt_estado[] = {
    "/* ---- estado do analisador ---- */",
    "",
    "static Token tok;                  /* token corrente (lookahead) */",
    "static int imprimir_arvore = 0;    /* opcao --tree */",
    "static int nivel_arvore = 0;",
    "static FILE *arquivo_tokens = NULL; /* opcao --tokens */",
    "",
    "static Token token_de_arquivo(void);",
    "",
    NULL
};

static const char *const rt_primitivas[] = {
    "/* ---- primitivas ---- */",
    "",
    "int pertence(int tipo, const int *conjunto)",
    "{",
    "    for (; *conjunto != -1; conjunto++)",
    "        if (*conjunto == tipo)",
    "            return 1;",
    "    return 0;",
    "}",
    "",
    "/* Reporta o erro com os tokens esperados (lista terminada em -1) e encerra. */",
    "void erro_sintatico(const int *esperados)",
    "{",
    "    int i;",
    "    escrever_posicao();",
    "    if ((int) tok.tipo == TOKEN_FIM)",
    "        fprintf(stderr, \"encontrado fim de arquivo, \");",
    "    else",
    "        fprintf(stderr, \"encontrado '%s', \", tok.lexema);",
    "    if (esperados[0] != -1 && esperados[1] == -1) {",
    "        fprintf(stderr, \"esperado %s\\n\", nome_token(esperados[0]));",
    "    } else {",
    "        fprintf(stderr, \"esperado um de {\");",
    "        for (i = 0; esperados[i] != -1; i++)",
    "            fprintf(stderr, \"%s%s\", i > 0 ? \", \" : \"\", nome_token(esperados[i]));",
    "        fprintf(stderr, \"}\\n\");",
    "    }",
    "    exit(EXIT_FAILURE);",
    "}",
    "",
    "static void recuo(void)",
    "{",
    "    int i;",
    "    for (i = 0; i < nivel_arvore; i++)",
    "        fputs(\"  \", stdout);",
    "}",
    "",
    "void arvore_entra(const char *nao_terminal)",
    "{",
    "    if (imprimir_arvore) {",
    "        recuo();",
    "        printf(\"%s\\n\", nao_terminal);",
    "    }",
    "    nivel_arvore++;",
    "}",
    "",
    "void arvore_sai(void)",
    "{",
    "    nivel_arvore--;",
    "}",
    "",
    "/* Consome o token corrente se for do tipo esperado; senao, erro. */",
    "void match(int esperado)",
    "{",
    "    int lista[2];",
    "    if ((int) tok.tipo != esperado) {",
    "        lista[0] = esperado;",
    "        lista[1] = -1;",
    "        erro_sintatico(lista);",
    "    }",
    "    if (SUPORTE_ARVORE && imprimir_arvore) {",
    "        recuo();",
    "        if (nome_token(tok.tipo)[0] == '\\'')",
    "            printf(\"%s\\n\", nome_token(tok.tipo));",
    "        else",
    "            printf(\"%s '%s'\\n\", nome_token(tok.tipo), tok.lexema);",
    "    }",
    "    avancar();",
    "}",
    "",
    NULL
};

/* Lexico proprio: partes fixas antes e depois das funcoes de regex. */
static const char *const rt_lexico_estado[] = {
    "/* ---- analisador lexico gerado a partir de TOKENS ---- */",
    "",
    "static char *fonte = NULL;",
    "static size_t pos = 0;",
    "static int linha = 1, coluna = 1;",
    "",
    "static char *ler_tudo(FILE *f)",
    "{",
    "    size_t cap = 4096, n = 0, lidos;",
    "    char *buf = (char *) malloc(cap), *novo;",
    "    if (buf == NULL) {",
    "        fprintf(stderr, \"memoria insuficiente\\n\");",
    "        exit(EXIT_FAILURE);",
    "    }",
    "    while ((lidos = fread(buf + n, 1, cap - n - 1, f)) > 0) {",
    "        n += lidos;",
    "        if (cap - n - 1 == 0) {",
    "            cap *= 2;",
    "            novo = (char *) realloc(buf, cap);",
    "            if (novo == NULL) {",
    "                fprintf(stderr, \"memoria insuficiente\\n\");",
    "                exit(EXIT_FAILURE);",
    "            }",
    "            buf = novo;",
    "        }",
    "    }",
    "    buf[n] = '\\0';",
    "    return buf;",
    "}",
    "",
    "static void consumir(size_t n)",
    "{",
    "    for (; n > 0 && fonte[pos] != '\\0'; n--, pos++) {",
    "        if (fonte[pos] == '\\n') {",
    "            linha++;",
    "            coluna = 1;",
    "        } else {",
    "            coluna++;",
    "        }",
    "    }",
    "}",
    "",
    NULL
};

static void escrever_cabecalho_c(FILE *f, const OpcoesGeracao *op)
{
    fputs("/*\n * Analisador sintatico descendente recursivo gerado a partir de ", f);
    emitir_comentario(f, op->origem);
    fputs(".\n * Arquivo gerado automaticamente: nao edite.\n */\n", f);
}

/* ------------------------------------------------------------------ */
/* Arquivo .h                                                         */
/* ------------------------------------------------------------------ */

static int gerar_h(const Gramatica *g, const OpcoesGeracao *op)
{
    char *caminho = (char *) xmalloc(strlen(op->prefixo) + 3);
    FILE *f;
    int i;

    sprintf(caminho, "%s.h", op->prefixo);
    f = fopen(caminho, "w");
    if (f == NULL) {
        diag_erro(0, 0, "nao foi possivel criar %s", caminho);
        free(caminho);
        return -1;
    }
    escrever_cabecalho_c(f, op);
    fputs("#ifndef PARSER_GERADO_H\n#define PARSER_GERADO_H\n\n", f);
    if (op->microc) {
        fputs("#include \"scanner.h\"   /* scanner do TP1: Token, TokenType, proximo_token */\n\n", f);
        fputs("#define TOKEN_FIM END_OF_FILE\n\n", f);
    } else {
        fputs("typedef enum {\n", f);
        for (i = 0; i < g->nterms; i++) {
            fprintf(f, "    %s%s   /* ", g->terms[i].nome_c, i + 1 < g->nterms ? "," : "");
            emitir_comentario(f, g->terms[i].exibicao);
            fputs(" */\n", f);
        }
        fputs("} TipoToken;\n\n", f);
        fputs("typedef struct {\n    int tipo;\n    char lexema[100];\n"
              "    int linha;\n    int coluna;\n} Token;\n\n", f);
        fputs("#define TOKEN_FIM T_FIM\n\n", f);
    }
    fputs("const char *nome_token(int tipo);\n", f);
    fputs("int  pertence(int tipo, const int *conjunto);\n", f);
    fputs("void avancar(void);\n", f);
    fputs("void match(int esperado);\n", f);
    fputs("void erro_sintatico(const int *esperados);\n", f);
    fputs("void arvore_entra(const char *nao_terminal);\n", f);
    fputs("void arvore_sai(void);\n\n", f);
    for (i = 0; i < g->nnts; i++)
        if (g->nts[i].regra != NULL)
            fprintf(f, "void parse_%s(void);\n", g->nts[i].nome);
    fputs("\n#endif\n", f);
    fclose(f);
    free(caminho);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Arquivo .c                                                         */
/* ------------------------------------------------------------------ */

static void gerar_nome_token(FILE *f, const Gramatica *g)
{
    int i;
    fputs("const char *nome_token(int tipo)\n{\n    switch (tipo) {\n", f);
    for (i = 0; i < g->nterms; i++) {
        fprintf(f, "    case %s: return ", g->terms[i].nome_c);
        emitir_string_c(f, i == TERM_FIM ? "fim de arquivo" : g->terms[i].exibicao);
        fputs(";\n", f);
    }
    fputs("    default: return \"token desconhecido\";\n    }\n}\n\n", f);
}

static void gerar_posicao_e_avancar(FILE *f, int microc)
{
    fputs("static void escrever_posicao(void)\n{\n", f);
    if (microc)
        fputs("    fprintf(stderr, \"linha %d: \", tok.linha);\n", f);
    else
        fputs("    fprintf(stderr, \"linha %d, coluna %d: \", tok.linha, tok.coluna);\n", f);
    fputs("}\n\n", f);

    fputs("/* Le o proximo token para tok. */\nvoid avancar(void)\n{\n", f);
    fputs("    if (arquivo_tokens != NULL)\n        tok = token_de_arquivo();\n    else\n", f);
    if (microc) {
        fputs("        tok = proximo_token();\n", f);
        fputs("    if (tok.tipo == UNDEF) {\n", f);
        fputs("        fprintf(stderr, \"linha %d: erro lexico: %s\\n\", tok.linha, tok.lexema);\n", f);
        fputs("        exit(EXIT_FAILURE);\n    }\n", f);
    } else {
        fputs("        tok = proximo_token_gerado();\n", f);
    }
    fputs("}\n\n", f);
}

static int gerar_lexico(FILE *f, const Gramatica *g)
{
    int i, nlit = 0, erros = 0;
    char nome[300];

    emit_linhas(f, rt_lexico_estado);

    for (i = 0; i < g->nskips; i++) {
        sprintf(nome, "casa_skip_%d", i);
        fputs("/* SKIP: ", f);
        emitir_comentario(f, g->skips[i]);
        fputs(" */\n", f);
        if (gen_casamento_regex(f, nome, g->skips[i]) != 0)
            erros++;
    }
    for (i = 1; i < g->nterms; i++) {
        if (g->terms[i].literal) {
            nlit++;
            continue;
        }
        sprintf(nome, "casa_%.250s", g->terms[i].nome_c);
        fprintf(f, "/* %s: ", g->terms[i].nome);
        emitir_comentario(f, g->terms[i].regex ? g->terms[i].regex : "");
        fputs(" */\n", f);
        if (gen_casamento_regex(f, nome, g->terms[i].regex ? g->terms[i].regex : "")
            != 0)
            erros++;
    }

    /* literais: comparacao direta, com prioridade em caso de empate */
    fputs("static const char *const texto_literais[] = {\n", f);
    for (i = 1; i < g->nterms; i++)
        if (g->terms[i].literal) {
            fputs("    ", f);
            emitir_string_c(f, g->terms[i].nome);
            fputs(",\n", f);
        }
    fputs("    NULL\n};\n\nstatic const int tipo_literais[] = {\n", f);
    for (i = 1; i < g->nterms; i++)
        if (g->terms[i].literal)
            fprintf(f, "    %s,\n", g->terms[i].nome_c);
    fputs("    -1\n};\n\n", f);

    fputs("/* Reconhece o maior prefixo da entrada; em caso de empate vence\n"
          " * o literal (palavras reservadas tem prioridade sobre ID). */\n", f);
    fputs("static Token proximo_token_gerado(void)\n{\n", f);
    fputs("    Token t;\n    size_t n, melhor;\n    int i, tipo, pulou;\n\n", f);
    fputs("    do {\n        pulou = 0;\n", f);
    for (i = 0; i < g->nskips; i++)
        fprintf(f, "        if ((n = casa_skip_%d(fonte + pos)) > 0) {\n"
                   "            consumir(n);\n            pulou = 1;\n        }\n", i);
    fputs("    } while (pulou);\n\n", f);
    fputs("    memset(&t, 0, sizeof t);\n", f);
    fputs("    t.linha = linha;\n    t.coluna = coluna;\n", f);
    fputs("    if (fonte[pos] == '\\0') {\n        t.tipo = T_FIM;\n        return t;\n    }\n", f);
    fputs("    melhor = 0;\n    tipo = -1;\n", f);
    fputs("    for (i = 0; texto_literais[i] != NULL; i++) {\n", f);
    fputs("        n = strlen(texto_literais[i]);\n", f);
    fputs("        if (n > melhor && strncmp(fonte + pos, texto_literais[i], n) == 0) {\n", f);
    fputs("            melhor = n;\n            tipo = tipo_literais[i];\n        }\n    }\n", f);
    for (i = 1; i < g->nterms; i++)
        if (!g->terms[i].literal)
            fprintf(f, "    if ((n = casa_%s(fonte + pos)) > melhor) {\n"
                       "        melhor = n;\n        tipo = %s;\n    }\n",
                    g->terms[i].nome_c, g->terms[i].nome_c);
    fputs("    if (melhor == 0) {\n", f);
    fputs("        fprintf(stderr, \"linha %d, coluna %d: erro lexico: caractere "
          "invalido '%c'\\n\",\n                linha, coluna, fonte[pos]);\n", f);
    fputs("        exit(EXIT_FAILURE);\n    }\n", f);
    fputs("    n = melhor < sizeof t.lexema ? melhor : sizeof t.lexema - 1;\n", f);
    fputs("    memcpy(t.lexema, fonte + pos, n);\n    t.lexema[n] = '\\0';\n", f);
    fputs("    t.tipo = tipo;\n    consumir(melhor);\n    return t;\n}\n\n", f);
    (void) nlit;
    return erros;
}

static void gerar_procedimentos(FILE *f, const Gramatica *g, const OpcoesGeracao *op)
{
    int i;
    Emissor em;
    em.f = f;
    em.nivel = 0;

    fputs("/* ---- procedimentos (um por nao terminal) ---- */\n\n", f);
    for (i = 0; i < g->nnts; i++) {
        if (g->nts[i].regra == NULL)
            continue;
        fprintf(f, "/* %s = ", g->nts[i].nome);
        {
            /* a regra em EBNF, sem "* /" */
            FILE *tmp = tmpfile();
            int c;
            if (tmp != NULL) {
                char buf[2] = { 0, 0 };
                expr_imprimir(tmp, g, g->nts[i].regra);
                rewind(tmp);
                while ((c = fgetc(tmp)) != EOF) {
                    buf[0] = (char) c;
                    emitir_comentario(f, buf);
                }
                fclose(tmp);
            }
        }
        fputs(" ; */\n", f);
        fprintf(f, "void parse_%s(void)\n{\n", g->nts[i].nome);
        em.nivel = 1;
        if (op->arvore)
            emit(&em, "arvore_entra(\"%s\");", g->nts[i].nome);
        gen_expr(&em, g, g->nts[i].regra);
        if (op->arvore)
            emit(&em, "arvore_sai();");
        fputs("}\n\n", f);
    }
}

static void gerar_main(FILE *f, const Gramatica *g, const OpcoesGeracao *op)
{
    fputs("/* ---- programa principal ---- */\n\n", f);
    fputs("int main(int argc, char **argv)\n{\n", f);
    fputs("    int i, fim[2];\n    const char *arquivo = NULL, *tokens = NULL;\n", f);
    fputs("    FILE *entrada = stdin;\n\n", f);
    fputs("    for (i = 1; i < argc; i++) {\n", f);
    fputs("        if (strcmp(argv[i], \"--tree\") == 0) {\n", f);
    fputs("            imprimir_arvore = 1;\n", f);
    fputs("        } else if (strcmp(argv[i], \"--tokens\") == 0 && i + 1 < argc) {\n", f);
    fputs("            tokens = argv[++i];\n", f);
    fputs("        } else if (argv[i][0] == '-' && argv[i][1] != '\\0') {\n", f);
    fputs("            fprintf(stderr, \"uso: %s [--tree] [--tokens arquivo.tok | "
          "programa]\\n\", argv[0]);\n", f);
    fputs("            return 2;\n        } else {\n            arquivo = argv[i];\n"
          "        }\n    }\n", f);
    fputs("    if (imprimir_arvore && !SUPORTE_ARVORE)\n", f);
    fputs("        fprintf(stderr, \"aviso: gere o analisador com --tree para "
          "imprimir a arvore\\n\");\n\n", f);
    fputs("    if (tokens != NULL) {\n", f);
    fputs("        arquivo_tokens = fopen(tokens, \"r\");\n", f);
    fputs("        if (arquivo_tokens == NULL) {\n", f);
    fputs("            fprintf(stderr, \"nao foi possivel abrir %s\\n\", tokens);\n", f);
    fputs("            return 2;\n        }\n    } else {\n", f);
    fputs("        if (arquivo != NULL && (entrada = fopen(arquivo, \"r\")) == NULL) {\n", f);
    fputs("            fprintf(stderr, \"nao foi possivel abrir %s\\n\", arquivo);\n", f);
    fputs("            return 2;\n        }\n", f);
    if (op->microc)
        fputs("        scanner_iniciar(entrada);\n", f);
    else
        fputs("        fonte = ler_tudo(entrada);\n", f);
    fputs("    }\n\n", f);
    fputs("    avancar();\n", f);
    fprintf(f, "    parse_%s();\n", g->nts[g->inicial].nome);
    fputs("    if (tok.tipo != TOKEN_FIM) {\n", f);
    fputs("        fim[0] = TOKEN_FIM;\n        fim[1] = -1;\n", f);
    fputs("        erro_sintatico(fim);\n    }\n", f);
    fputs("    printf(\"entrada aceita\\n\");\n\n", f);
    fputs("    if (arquivo_tokens != NULL) {\n        fclose(arquivo_tokens);\n", f);
    fputs("    } else {\n", f);
    if (op->microc)
        fputs("        scanner_finalizar();\n", f);
    else
        fputs("        free(fonte);\n", f);
    fputs("        if (entrada != stdin)\n            fclose(entrada);\n    }\n", f);
    fputs("    return 0;\n}\n", f);
}

static const char *nome_base(const char *caminho)
{
    const char *b = strrchr(caminho, '/');
    return b != NULL ? b + 1 : caminho;
}

static int gerar_c(const Gramatica *g, const OpcoesGeracao *op)
{
    char *caminho = (char *) xmalloc(strlen(op->prefixo) + 3);
    FILE *f, *corpo;
    int c, erros = 0;

    /* Os procedimentos sao gerados primeiro num arquivo temporario porque
     * registram os conjuntos (conj_N), que precisam vir antes no .c. */
    corpo = tmpfile();
    if (corpo == NULL) {
        diag_erro(0, 0, "nao foi possivel criar arquivo temporario");
        free(caminho);
        return -1;
    }
    gerar_procedimentos(corpo, g, op);

    sprintf(caminho, "%s.c", op->prefixo);
    f = fopen(caminho, "w");
    if (f == NULL) {
        diag_erro(0, 0, "nao foi possivel criar %s", caminho);
        fclose(corpo);
        free(caminho);
        return -1;
    }
    escrever_cabecalho_c(f, op);
    fputs("#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n", f);
    fprintf(f, "#include \"%s.h\"\n\n", nome_base(op->prefixo));
    fprintf(f, "#define SUPORTE_ARVORE %d\n\n", op->arvore ? 1 : 0);

    fputs("/* ---- conjuntos de tokens ---- */\n\n", f);
    emitir_conjuntos(f, g);
    fputc('\n', f);

    emit_linhas(f, rt_estado);
    if (!op->microc)
        erros += gerar_lexico(f, g);
    gerar_nome_token(f, g);
    gerar_posicao_e_avancar(f, op->microc);
    emit_linhas(f, rt_primitivas);
    gen_token_de_arquivo(f, op->microc);

    rewind(corpo);
    while ((c = fgetc(corpo)) != EOF)
        fputc(c, f);
    fclose(corpo);

    gerar_main(f, g, op);
    fclose(f);
    free(caminho);
    return erros == 0 ? 0 : -1;
}

int codegen_gerar(Gramatica *g, const OpcoesGeracao *op)
{
    int r;
    nomear_terminais(g, op->microc);
    r = gerar_h(g, op);
    if (r == 0)
        r = gerar_c(g, op);
    liberar_conjuntos();
    return r;
}
