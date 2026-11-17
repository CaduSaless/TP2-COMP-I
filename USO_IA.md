# USO_IA.md

Registro de cada uso relevante de ferramentas de IA generativa, no formato do TP1. Em todas as entradas a ferramenta foi a mesma: Claude Code (Anthropic), modelo Claude Sonnet 5.5.

O uso teve duas fases:

- **Fase 1, durante a implementação (entradas 1 a 8):** a ferramenta foi usada como tutor, para esclarecer dúvidas conceituais e da linguagem C sobre código que o próprio esqueleto já fornecia. Nenhum trecho dos `TODO(aluno)` de `src/` foi copiado de uma resposta da IA.
- **Fase 2, finalização (entradas 9 a 13):** a ferramenta foi usada para entender o código já pronto, escrever a extensão da árvore gráfica, completar os testes e melhorar a documentação. Nessa fase a IA escreveu código e texto, e isso está dito em cada entrada.

Modelo de entrada:

    Ferramenta: <ferramenta e modelo>
    Trecho:     <arquivo ou parte do trabalho>
    Finalidade: <o que foi pedido, de forma especifica>
    O que fiz:  <usado integralmente, adaptado ou apenas consultado>

## Fase 1: dúvidas durante a implementação

### Entrada 1

    Ferramenta: Claude Code (Claude Sonnet 5.5)
    Trecho:     src/ebnf_lexer.h e src/ebnf_lexer.c (struct MetaLexer, funções mlex_iniciar e
                mlex_proximo)
    Finalidade: Entender para que serve o struct MetaLexer e o que cada campo representa: fonte
                (o arquivo .ebnf inteiro lido como uma única string terminada em '\0'), pos
                (índice do caractere atual), linha e coluna (contadores usados só nas mensagens
                de erro). Entender que a coluna conta caracteres desde o início da linha, e não
                palavras.
    O que fiz:  Apenas consultei. Li o código fornecido com a explicação como apoio e não alterei
                o módulo, que já vem pronto.

### Entrada 2

    Ferramenta: Claude Code (Claude Sonnet 5.5)
    Trecho:     src/ebnf_lexer.c (funções atual, seguinte, avancar, criar, pular_brancos,
                ler_literal, ler_regex, ler_identificador)
    Finalidade: Entender o papel de cada função auxiliar e por que elas não aparecem no .h.
                Entender como o mlex_proximo escolhe qual função de leitura chamar a partir do
                primeiro caractere. Entender a diferença entre literal (texto fixo entre aspas,
                com escapes interpretados e usado nas regras) e regex (padrão entre barras,
                mantido como está e usado em TOKENS).
    O que fiz:  Apenas consultei, para compreender o funcionamento do analisador léxico da
                notação.

### Entrada 3

    Ferramenta: Claude Code (Claude Sonnet 5.5)
    Trecho:     src/ebnf_lexer.h (comentário "erros léxicos já são reportados"),
                src/ebnf_parser.c (função avancar) e src/util.c (diag_erro)
    Finalidade: Entender por que o lexer reporta o erro por conta própria, devolve um token
                MT_ERRO e o parser o descarta sem reportar de novo. Entender como o contador de
                erros faz o gerador falhar no final mesmo assim.
    O que fiz:  Apenas consultei.

### Entrada 4

    Ferramenta: Claude Code (Claude Sonnet 5.5)
    Trecho:     src/ebnf_lexer.c, src/ebnf_lexer.h e src/gramatica.h (ponteiros MetaLexer *lx,
                const char *fonte, MetaToken *t)
    Finalidade: Dúvidas da linguagem C: o que é um ponteiro, por que structs são passadas por
                ponteiro (em C os argumentos são passados por cópia, e a função precisa alterar
                o original), a diferença entre . e ->, o uso de & ao passar &p.lx, o significado
                de const e como uma string em C é uma sequência de char terminada em '\0'.
    O que fiz:  Apenas consultei, para poder ler e escrever o restante do código C do trabalho.

### Entrada 5

    Ferramenta: Claude Code (Claude Sonnet 5.5)
    Trecho:     Organização em pares .h e .c de todo o projeto (src/), incluindo
                microc/tokens.h
    Finalidade: Dúvidas da linguagem C: a diferença entre interface (.h, declarações) e
                implementação (.c), o papel de static (função visível só no arquivo), de extern
                (declara algo definido em outro arquivo) e do include guard (#ifndef / #define /
                #endif), que evita redefinição quando um header é incluído mais de uma vez.
    O que fiz:  Consultei a explicação e escrevi o include guard no microc/tokens.h.

### Entrada 6

    Ferramenta: Claude Code (Claude Sonnet 5.5)
    Trecho:     src/gramatica.h e src/ebnf_parser.c (cabeçalho com a gramática da notação,
                tokens MT_NOME_TERM, MT_LITERAL, MT_NAOTERM, struct Parser, gram_terminal)
    Finalidade: Entender a diferença entre terminal nomeado, literal e não terminal, e como cada
                um é guardado na struct Gramatica (tabelas terms e nts referenciadas por
                índice). Entender os dois níveis de entrada do projeto: o arquivo .ebnf lido
                pelo gerador e o programa lido pelo analisador gerado.
    O que fiz:  Apenas consultei, como preparação para implementar parse_expr, parse_seq e
                parse_fator, que o grupo escreveu depois.

### Entrada 7

    Ferramenta: Claude Code (Claude Sonnet 5.5)
    Trecho:     microc/microc.flex, microc/tokens.h e microc/scanner.c (adaptação do scanner do
                TP1)
    Finalidade: Entender %option prefix="microc_yy" e por que os nomes microc_yylex,
                microc_yyin, microc_yytext e microc_yylex_destroy mudam. Entender o papel de
                valor_atual, a diferença entre o texto bruto de microc_yytext e o valor
                processado do TP1, por que a main virou proximo_token, por que valor_atual deve
                ser zerado a cada chamada, por que ultimo_token deve ser atualizado (regra do
                inteiro negativo) e para que serve tabela_liberar.
    O que fiz:  Adaptei ao meu TP1. Removi a main e o yywrap, movi o enum para tokens.h,
                acrescentei a struct Token, tirei o static de valor_atual e ultimo_token, e
                escrevi tabela_liberar. Em scanner.c, ajustei proximo_token e scanner_finalizar
                e escrevi texto_do_lexema.

### Entrada 8

    Ferramenta: Claude Code (Claude Sonnet 5.5)
    Trecho:     Visão geral do TP2 (README.md do pacote, src/main.c, Makefile)
    Finalidade: Entender o que é EBNF, o objetivo de cada etapa do gerador e a ordem de
                implementação dos TODO(aluno), de acordo com as dependências entre elas (leitura
                da EBNF, conjuntos, LL(1), verificação, geração).
    O que fiz:  Usei a ordem sugerida como plano de estudo. Também comparei a estrutura de
                parse_expr, parse_seq e parse_fator.

## Fase 2: finalização (código já implementado)

### Entrada 9: leitura e explicação do código pronto

    Ferramenta: Claude Code (Claude Sonnet 5.5)
    Trecho:     src/ (ebnf_parser.c, conjuntos.c, verificacao.c, ll1.c, codegen.c) e
                microc/scanner.c
    Finalidade: Pedimos uma explicação do que cada módulo faz (FIRST e FOLLOW por ponto fixo,
                recursão à esquerda pelo fecho transitivo, condições LL(1), tradução das regras
                e léxico por AFN e AFD), para estudar o código antes da apresentação.
    O que fiz:  Apenas consultado. Nenhuma linha foi alterada nesta etapa.

### Entrada 10: árvore de derivação gráfica (opção --svg)

    Ferramenta: Claude Code (Claude Sonnet 5.5)
    Trecho:     src/codegen.c: vetor rt_svg (código C que vai para o analisador gerado), as
                chamadas acrescentadas em arvore_entra e match, e a opção --svg em gerar_main
    Finalidade: Gerar um desenho da árvore de derivação em SVG, sem bibliotecas externas, para o
                ponto extra. A IA escreveu o código: registro dos nós em pré-ordem, cálculo das
                posições (cada subárvore guarda o limite esquerdo e o direito de cada nível) e
                escrita do arquivo, com escape de XML nos lexemas.
    O que fiz:  Usado integralmente. Testado com make test (a saída de 2 * x + (3 - y) é
                comparada com testes/arvore/expr.svg), compilado com -ansi -pedantic -Wall
                -Wextra sem avisos e verificado com valgrind --leak-check=full, sem vazamentos,
                nas gramáticas expr, json e Micro C. Também conferimos o desenho visualmente.

### Entrada 11: gramática JSON e testes da árvore

    Ferramenta: Claude Code (Claude Sonnet 5.5)
    Trecho:     testes/gramaticas/json.ebnf, testes/programas/json/, testes/esperado/json.gerador,
                testes/arvore/ e o bloco da árvore em testes/executar.sh
    Finalidade: Criar a terceira gramática válida exigida pelo enunciado (JSON simplificado), com
                3 programas aceitos e 3 rejeitados, e testes automáticos para --tree e --svg.
    O que fiz:  Usado integralmente. As saídas esperadas foram geradas pelo próprio analisador e
                conferidas uma a uma (mensagens, linhas e colunas).

### Entrada 12: pequenos ajustes no código

    Ferramenta: Claude Code (Claude Sonnet 5.5)
    Trecho:     src/verificacao.c (mensagem do aviso de não terminal inalcançável), comentários
                de cabeçalho dos arquivos de src/, microc/LEIAME.md e .gitignore
    Finalidade: Deixar o aviso igual ao exemplo do manual ("f.ebnf:6:3: aviso: nao terminal sobra
                inalcancavel a partir de lista") e trocar os comentários "TODO(aluno)" que
                restavam, porque o checklist de entrega pede que nenhum sobre.
    O que fiz:  Usado integralmente.

### Entrada 13: documentação e relatório

    Ferramenta: Claude Code (Claude Sonnet 5.5)
    Trecho:     README.md, USO_IA.md e relatorio/relatorio.html (que gera relatorio.pdf)
    Finalidade: Reescrever o README em formato de passo a passo, organizar este registro e
                redigir o relatório técnico a partir do código e dos testes do repositório.
    O que fiz:  Adaptado a partir do README do pacote e revisado. As mensagens de erro, a
                contagem de testes e a regra de desempate do léxico citadas nos textos foram
                conferidas executando o gerador.
