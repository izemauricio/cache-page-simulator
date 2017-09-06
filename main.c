// trabalho organizacao de computadores - ufrgs - 2016.2 - mauricio

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define M_LOG2E 1.44269504088896340736 //log2(e)

//coloque a pasta INPUT no C:
#define FILE1 "C:/INPUT/cachedesc_A.dat"
#define FILE2 "C:/INPUT/input_A.dat"
#define FILE3 ""

// Windows unsigned long long = %llu   Linux = %Lu

inline unsigned long long log2(const unsigned int x)
{
    //double xx = (double) x;
    //return  ceil(log(x) * M_LOG2E);

    //log(n)/log(2) is log2.
    return ceil(log(x) / log(2));
}


struct CACHEDATA
{
    unsigned long long tag;

    int count;
    int valid;
    unsigned long long lru_control;
    unsigned long long fifo_control;
};

struct RESULTADO
{
    int write_hit;
    int write_miss;
    int read_hit;
    int read_miss;
} resultado;

int main()
{
    printf("Hi!\n");

    printf("size of long long = %d bits\n", sizeof(unsigned long long) * 8);
    //unsigned long long t = 140735601943344L;
    //printf("%llu\n",t);

    // Abre arquivo e alimenta essas variaveis
    unsigned int line_size;
    unsigned int number_of_lines;
    unsigned int associativity;
    char replacement_policy[5];

    FILE *fp;
    char buff[255];

    fp = fopen(FILE1, "r");

    if(fp==0)
    {
        printf("Erro ao abrir o arquivo!\n");
        getchar();
        return -1;
    }

    fscanf(fp, "line size = %u\n", &line_size);
    printf("line size: %d\n", line_size);

    fscanf(fp, "number of lines = %u\n", &number_of_lines);
    printf("number of lines: %d\n", number_of_lines);

    fscanf(fp, "associativity = %u\n", &associativity);
    printf("associativity: %d\n", associativity);

    fscanf(fp, "replacement policy = %s\n", replacement_policy);
    printf("replacement_policy: %s\n", replacement_policy);

    fclose(fp);

    // Create the cache structure
    int numero_de_sets = number_of_lines / associativity;
    int numero_de_lines_per_set = associativity;

    struct CACHEDATA*** cache;
    int set;
    int line = 0;

    // Aloca N sets contendo um ponteiro para outro ponteiro
    cache = (struct CACHEDATA***) malloc( numero_de_sets * sizeof(struct CACHEDATA**));
    if ( cache == NULL )
    {
        printf("Falta de memoria 1!\n");
        getchar();
        return -1;
    }

    // Para cada set da cache
    for ( set = 0; set < numero_de_sets; set++ )
    {

        // Aloca N linhas contendo um ponteiro para struct
        cache[set] = (struct CACHEDATA**) malloc( numero_de_lines_per_set * sizeof(struct CACHEDATA*) );
        if ( cache[set] == NULL )
        {
            printf("Falta de memoria 2!\n");
            getchar();
            return -1;
        }

        // Para cada linha, aloca uma struct inteira
        for ( line=0; line < numero_de_lines_per_set; line++ )
        {
             struct CACHEDATA* data = (struct CACHEDATA*) malloc(sizeof(struct CACHEDATA));

             data->count = 0;
             data->tag = 0;
             data->valid = 0;
             data->fifo_control = 0;
             data->lru_control = 0;

             cache[set][line] = data;
        }

    }

    // Testar
    //int info = (cache[0][0])->count;
    //printf("count=%d\n", info);
    //getchar();

    // Pega cada um dos addr do arquivo
    fp = fopen(FILE2, "r");

    if(fp==0)
    {
        printf("Erro ao abrir o arquivo 2!\n");
        getchar();
        return -1;
    }

    //char addr[50];
    unsigned long long addr;
    char operation[10];
    int count = 0;

    //fseek(fp, 1, SEEK_SET);

    while(1)
    {
        int fscan_return;

        //fscan_return = fscanf(fp, "%s", addr);
        fscan_return = fscanf(fp, "%llu", &addr);

        if(fscan_return == EOF)
            break;

        fscan_return = fscanf(fp, "%s", operation);

        if(fscan_return == EOF)
            break;

        //printf("[%llu][%s]\n", addr, operation);

        count++;

        // Processa cada um dos addr do arquivo

        unsigned long long cpu_addr_tag = addr >> log2(line_size);
        unsigned long long cpu_addr_set = cpu_addr_tag % numero_de_sets;

        // Percore todas as linhas do set a procura do tag
        int hit = 0;
        for ( line=0; line < numero_de_lines_per_set; line++ )
        {
             if(  (cache[cpu_addr_set][line])->tag == cpu_addr_tag  )
             {
                 // LRU record - Contar o numero de acessos
                 (cache[cpu_addr_set][line])->lru_control++;

                 // Hit record
                 if( strcmp(operation,"R")==0 )
                    resultado.read_hit++;
                 else
                    resultado.write_hit++;

                hit = 1;

                break;
             }
        }

        if(hit==0)
        {
                // Miss record
                if( strcmp(operation,"R")==0 )
                    resultado.read_miss++;
                else
                    resultado.write_miss++;

                // Substitui:

                // Tenta achar uma linha com valid bit == 0
                int achou = 0;
                for ( line=0; line < numero_de_lines_per_set; line++ )
                {
                    if(  (cache[cpu_addr_set][line])->valid == 0  )
                     {

                          // Insere
                         (cache[cpu_addr_set][line])->tag = cpu_addr_tag;
                         (cache[cpu_addr_set][line])->valid = 1;
                         (cache[cpu_addr_set][line])->lru_control = 0;
                         // FIFO: Sou o ultimo a ser inserido no set
                          long long ultimo_inserido = -1;
                          int line2 = 0;
                          for ( line2=0; line2 < numero_de_lines_per_set; line2++ )
                          {
                                if(ultimo_inserido < (cache[cpu_addr_set][line2])->fifo_control)
                                    ultimo_inserido = (cache[cpu_addr_set][line2])->fifo_control;
                          }
                         (cache[cpu_addr_set][line])->fifo_control = ultimo_inserido+1;
                        // Fim - Insere

                         achou = 1;

                         break;
                     }
                }

                if(achou==0)
                {
                     // FIFO - substitui o primeiro que foi inserido (a menor posicao)
                    if( strcmp(replacement_policy, "FIFO")==0 )
                    {
                        // Acho a linha que tem a menor posicao de insercao
                         unsigned long long menor_posicao = 99999999L;
                         for ( line=0; line < numero_de_lines_per_set; line++ )
                          {
                                if(menor_posicao > (cache[cpu_addr_set][line])->fifo_control)
                                    menor_posicao = (cache[cpu_addr_set][line])->fifo_control;
                          }

                          // Tiro ela fora e insiro a nova linha
                          for ( line=0; line < numero_de_lines_per_set; line++ )
                          {
                                if(menor_posicao == (cache[cpu_addr_set][line])->fifo_control)
                                {

                                    // Insere
                                    (cache[cpu_addr_set][line])->tag = cpu_addr_tag;
                                    (cache[cpu_addr_set][line])->lru_control = 0;
                                    // FIFO: Sou o ultimo a ser inserido no set
                                    int ultimo_inserido = -1;
                                    int line2 = 0;
                                    for ( line2=0; line2 < numero_de_lines_per_set; line2++ )
                                    {
                                    if(ultimo_inserido < (cache[cpu_addr_set][line2])->fifo_control)
                                        ultimo_inserido = (cache[cpu_addr_set][line2])->fifo_control;
                                    }
                                    (cache[cpu_addr_set][line])->fifo_control = ultimo_inserido+1;
                                    // Fim - Insere

                                    break;
                                }
                          }
                    }

                    // LRU - substitui o menos acessado
                    else
                    {
                        // Acho a linha que tem o menor numero de acessos
                        unsigned long long menor_posicao = 9999999L;
                        for ( line=0; line < numero_de_lines_per_set; line++ )
                        {
                            if(menor_posicao > (cache[cpu_addr_set][line])->lru_control)
                                menor_posicao = (cache[cpu_addr_set][line])->lru_control;
                        }

                        // Tiro ela fora e insiro no lugar
                          for ( line=0; line < numero_de_lines_per_set; line++ )
                          {
                                if(menor_posicao == (cache[cpu_addr_set][line])->lru_control)
                                {

                                    // Insere
                                    (cache[cpu_addr_set][line])->tag = cpu_addr_tag;
                                    (cache[cpu_addr_set][line])->lru_control = 0;
                                    // FIFO: Sou o ultimo a ser inserido no set
                                    int ultimo_inserido = -1;
                                    int line2 = 0;
                                    for ( line2=0; line2 < numero_de_lines_per_set; line2++ )
                                    {
                                    if(ultimo_inserido < (cache[cpu_addr_set][line2])->fifo_control)
                                        ultimo_inserido = (cache[cpu_addr_set][line2])->fifo_control;
                                    }
                                    (cache[cpu_addr_set][line])->fifo_control = ultimo_inserido+1;
                                    // Fim insere

                                    break;
                                }
                          }
                          // Fim - Tiro ela fora e insiro no lugar

                    }
                }
        }

        //getchar();
    }

    /*
    fscanf(fp, "%c", &operation);
    printf("operation: [%c]\n", operation);

    fscanf(fp, "%c", &operation);
    printf("operation: [%c]\n", operation);
    */

    //fgets(buff, 255, (FILE*)fp);
    //printf("3: %s\n", buff );

    fclose(fp);

    printf("Access count:%d\n", count);
    printf("Read hits:%d\n", resultado.read_hit);
    printf("Read misses:%d\n", resultado.read_miss);
    printf("Write hits:%d\n", resultado.write_hit);
    printf("Write misses:%d\n", resultado.write_miss);

    getchar();

    return 0;
}
