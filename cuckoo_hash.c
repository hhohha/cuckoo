/*
Autor: Jan Hammer, 3BIT
Projekt: Implementace cuckoo hash, ISP/IBP
Posl. modifkace: 14.10 08
*/

#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<limits.h>
#include<string.h>

/*error messages*/
typedef enum{
OK, ERR_MALLOC, HTABLE_CYCLE, ERR_BUILD, ERR
}ERRORS;

typedef struct {
  char str[30];
} HTableItem;

typedef struct Htable{
  int size;
  HTableItem *HT;
  int seedCnt;
  int *seeds;
}*HTABLE;


/*******************************************************     GLOBAL VARIABLES    *********************************************************/

FILE *logF, *inputF, *outF, *resF;

int build = 0;
int uns_build = 0;
int store = 0;
int move = 0;
int hashf = 0;
int input = 0;


int ftest = 0;
/*******************************************************   FUNCTION PROTOTYPES   *********************************************************/

int HashF(const char *str, unsigned range, int seed);

int HTablePrint(HTABLE HTable);
int HTableRebuild(HTABLE HTable);
int HTableDrop(HTABLE HTable);
int HTableStore(HTABLE HTable, HTableItem in);
int HTableMove(HTABLE HTable, HTableItem in, int from);
int HTableRemove(HTABLE HTable, HTableItem out);
int HTableInit(HTABLE HTable);
int HTableGenerateSeeds(HTABLE HTable);

int HTableStoreRand(HTABLE HTable, HTableItem in);
int HTableMoveRand(HTABLE HTable, HTableItem in);

int StoreStats();
void PrintHelp();

/*******************************************************        FUNCTIONS        *********************************************************/

int HashF(const char *str, unsigned range, int seed){

  hashf++;

  unsigned int h = 0;
  unsigned char *p;

  for(p = (unsigned char*)str; *p != '\0'; p++) h = seed * h + *p;

  return h % range;
}

/*---------------------------------------------------------------------------------------------------------------------------------------*/

void PrintHelp(){
  printf("Program Hash - implementace algoritmu cuckoo hash pro n hashovacich funkci\n");
  printf("parametr 1 - velikost hashovaci tabulky\n");
  printf("paramert 2 - pocet hashovacich funkci (min. 2)\n");
  printf("parametr 3 - specifikace vstupniho souboru\n");
}

/*---------------------------------------------------------------------------------------------------------------------------------------*/

int HTableGenerateSeeds(HTABLE HTable){

  for(int i = 0; i < HTable->seedCnt; i++)
    HTable->seeds[i] = rand() % 1000;

  fprintf(logF, "New seeds: ");			//ladci vypis
   for(int i = 0; i < HTable->seedCnt; i++)	//
    fprintf(logF, "%d, ", HTable->seeds[i]);	//
  fprintf(logF, "\n");				//

  return OK;
}

/*---------------------------------------------------------------------------------------------------------------------------------------*/

int HTableInit(HTABLE HTable){

  fprintf(logF, "Calling HTableInit\n");		//ladici vypis

  							//inicializace nove Hash tabulky
  if((HTable->HT = malloc(HTable->size * sizeof(HTableItem))) == NULL) return ERR_MALLOC;
  for(int i = 0; i < HTable->size; i++)HTable->HT[i].str[0] = '\0';

  return OK;

}

/*---------------------------------------------------------------------------------------------------------------------------------------*/

int HTablePrint(HTABLE HTable){

  fprintf(logF, "Calling HTablePrint\n");		//ladici vypis

  int cnt = 0;

  printf("Htable:\n---------------\nindex - value\n\n");
  for(int i = 0; i < HTable->size; i++){

    if(HTable->HT[i].str[0] == '\0') { printf("%d - XXX\n", i); fprintf(outF, "%d - XXX\n", i); }
    else { fprintf(stdout, "%d - %s\n", i, HTable->HT[i].str); fprintf(outF, "%d - %s\n", i, HTable->HT[i].str); }

    if(HTable->HT[i].str[0] != '\0') cnt++;
  }
  printf("\nHash table size: %d\n", HTable->size);
  printf("\nItems in hash table: %d\n", cnt);
  printf("\nHash table saturation: %f\n\n", cnt/(double)HTable->size);
  printf("Number of hash function: %d\n\n", HTable->seedCnt);
  printf("HTableRebuild() called: %dx, unsuccessful: %d\n", build, uns_build);
  printf("HTableStore() called: %dx\n", store);
  printf("HTableMove() called: %dx\n", move);
  printf("HashF() called: %dx\n\n\n", hashf);

  return OK;
}
/*---------------------------------------------------------------------------------------------------------------------------------------*/

int StoreStats(){

  FILE *f;

  int HashFNo, inputsNo, tableSize;
  double tableSat, avgStoreCalls, avgMoveCalls, avgBuildCalls, avgUnsuccBuilds;
  int ExecNo;


  f = fopen("stats", "r");

  fscanf(f, "%d %d %d %lf %lf %lf %lf %lf %d",
    &HashFNo, &inputsNo, &tableSize, &tableSat, &avgStoreCalls, &avgMoveCalls, &avgBuildCalls, &avgUnsuccBuilds, &ExecNo);

  fclose(f);

  tableSat = (double)inputsNo / tableSize;

  avgStoreCalls = (avgStoreCalls * ExecNo + store) / (ExecNo + 1);
  avgMoveCalls = (avgMoveCalls * ExecNo + move) / (ExecNo + 1);
  avgBuildCalls = (avgBuildCalls * ExecNo + build) / (ExecNo + 1);
  avgUnsuccBuilds = (avgUnsuccBuilds * ExecNo + uns_build) / (ExecNo + 1);

  ExecNo++;

  f = fopen("stats", "w");

  fprintf(f, "%d\n%d\n%d\n%f\n%f\n%f\n%f\n%f\n%d",
    HashFNo, inputsNo, tableSize, tableSat, avgStoreCalls, avgMoveCalls, avgBuildCalls, avgUnsuccBuilds, ExecNo);

  fclose(f);

  return OK;
}
/*---------------------------------------------------------------------------------------------------------------------------------------*/

int HTableRebuild(HTABLE HTable){

  fprintf(logF, "Calling HTableRebuild\n");		//ladici vypis
  fprintf(logF, "Rebuilding.....\n");
  build++;

  if(build == 2000){
    HTablePrint(HTable);
    printf("Rebuild not possible\n");
    exit(ERR);
  }

  HTABLE HTableNew;							//vytvoreni nove tabulky

  if((HTableNew = malloc(sizeof(struct Htable))) == NULL){
    fprintf(stderr, "err: malloc error\n");
    exit(ERR);
  }

  HTableNew->size = HTable->size;					//inicializace nove tabulky - hlavicka
  HTableNew->seedCnt = HTable->seedCnt;					//
  if((HTableNew->seeds = malloc(HTableNew->seedCnt * sizeof(int))) == NULL){
    fprintf(stderr, "err: malloc error\n");
    exit(ERR);
  }
  HTableGenerateSeeds(HTableNew);						//

  HTableInit(HTableNew);						//inicializace samotne tabulky


  for(int i = 0; i < HTable->size; i++){
    if(HTable->HT[i].str[0] == '\0') continue;

    if(HTableStore(HTableNew, HTable->HT[i]) != OK){		//pri preskladavani doslo k zacykleni => zrusit novou tabulku
     								//a vratit chybu, resi se dale v mainu
      for(int i = 0; i < HTable->seedCnt; i++)
        HTable->seeds[i] = HTableNew->seeds[i];

      fprintf(logF, "Rebuild not successful\n");
      uns_build++;

      free(HTableNew->HT);
      free(HTableNew->seeds);
      free(HTableNew);

      return ERR_BUILD;
    }
  }

  free(HTable->seeds);
  HTable->seeds = HTableNew->seeds;

  free(HTable->HT);
  HTable->HT = HTableNew->HT;

  free(HTableNew);

  return OK;
}
/*---------------------------------------------------------------------------------------------------------------------------------------*/

int HTableDrop(HTABLE HTable){

  fprintf(logF, "Calling HTableDrop\n");		//ladici vypis

  free(HTable->HT);
  free(HTable->seeds);

  return OK;
}

/*---------------------------------------------------------------------------------------------------------------------------------------*/

int HTableMove(HTABLE HTable, HTableItem in, int from){

  fprintf(logF, "   -> Calling HTableMove: %s from %d...    ", in.str, from);		//ladici vypis
  move++;

  static char firstMoveItem[30];				//promenne pro zachyceni 1. vkladaneho prvku a detekci zacykleni
  static int firstMoveFunct = -1;
  static int firstMoveIndex = -1;

  static int moveChain = 0;

  int retCode = OK;
  int *HF;							//vysleky hashovacich funkci

  int FunctNo = -1;						//konecna pouzita fce (-1 => zatim nenalezeno)
  int freeSlotFound = 0;					//byl nalezen volny slot?

  if(moveChain++ == 50000) { HTablePrint(HTable); printf("\nERROR: UNDETECTED CYCLE\n"); exit(ERR);}		//zachranna brzda

  if((HF = malloc(HTable->seedCnt * sizeof(int))) == NULL){		//alokace pameti pro vysledky hash fci
    fprintf(stderr, "err: malloc error\n");
    exit(ERR);
  }

  for(int i = 0; i < HTable->seedCnt; i++){			//pro vkladany prvek se zkousi postupne vsechny hash fce
    HF[i] = HashF(in.str, HTable->size, HTable->seeds[i]);	//a hleda se volny slot
    for(int j = i - 1; j >= 0; j--)				//zamezeni dvou stejnych indexu, takovy se nahradi
      if(HF[i] == HF[j]){HF[i] = -1; fprintf(logF, "  Funct no %d invalidated.   ", i); }		// -1 a nepouzije se

    if(HF[i] != -1 && HTable->HT[HF[i]].str[0] == '\0'){
      freeSlotFound = 1;					//volny slot nalezen (neni potreba dalsi move)
      FunctNo = i;						//pouzije se i-ta fce
      break;
    }
  }

  if(FunctNo == -1){						//nebyla-li konecna fce nalezena (tzn nebyl volny slot),
    for(int i = 0; i < HTable->seedCnt; i++)			//hleda se fce, kterou byla polozka naposledy ulozena
      if(HF[i] != -1 && HF[i] == from){				//tzn. fce odkazujici do from
        FunctNo = i;						//a je pouzita nasledujici fce (fce nulta, pokud jich vic uz neni)
        do{							//
          FunctNo = (FunctNo + 1 >= HTable->seedCnt) ? 0 : FunctNo + 1;	//preskakuji se naplatne indexy s hodnotou -1
        }while(HF[FunctNo] == -1);
        break;
      }
  }

  if(firstMoveIndex == -1){					//je-li tento Move prvni (tzn. volan fci Store, firstMoveIndex == 1-),
    strcpy(firstMoveItem, in.str);				//jsou ulozeny udaje co bylo ulozeno, kam a kterou fci
    firstMoveIndex = HF[FunctNo];				//neni-li Move prvni nasleduje test na zacykleni
    firstMoveFunct = FunctNo;

    fprintf(logF, "first move --> stored: moving Item - %s, Dest - %d, Funct - %d\n", firstMoveItem, firstMoveIndex, firstMoveFunct);
  }
/*   ------------   TEST NA ZACYKLENI ------------    */
//zacykleni nastava, je-li poprve presouvany prvek presouvan znovu na stejne misto, stejnou fci

  else if(strcmp(firstMoveItem, in.str) == 0 && firstMoveIndex == HF[FunctNo] && firstMoveFunct == FunctNo){

/*   ------------  RESENI ZACYKLENI   ------------    */
    for(int i = 0; i < HTable->size; i++){			//prvek se ulozi do 1. volneho slotu a nasleduje prestaveni tabulky
      if(HTable->HT[i].str[0] == '\0'){				//neni-li zadny volny slot, vkladany prvek se zahodi
        HTable->HT[i] = in;
        break;
      }
    }

    fprintf(logF, "cycle detected, rebuild neccessary\n\n");		//ladici vypis

    free(HF);
    return HTABLE_CYCLE;					//je potreba prestavet tabulku
  }
/*   ---------------------------------------------    */

  fprintf(logF, "move completed to %d ... funct. no: %d\n", HF[FunctNo], FunctNo);		//ladici vypis

  if(freeSlotFound == 1) HTable->HT[HF[FunctNo]] = in;		//byl-li drive nalezen volny slot, proved ulozeni
    else{
      HTableItem ToBMoved;					//jinak se ulozi, misto jineho, jiz drive vybraneho prvku
								//a ten se vlozi rekurzivne do dalsiho Move
      ToBMoved = HTable->HT[HF[FunctNo]];
      HTable->HT[HF[FunctNo]] = in;
      retCode = HTableMove(HTable, ToBMoved, HF[FunctNo]);
  }

  firstMoveIndex = -1;
  firstMoveFunct = -1;
  moveChain = 0;
  free(HF);

  return retCode;

}
/*---------------------------------------------------------------------------------------------------------------------------------------*/

int HTableMoveRand(HTABLE HTable, HTableItem in){

  fprintf(logF, "   -> Calling HTableMoveRand: %s...    \n", in.str);		//ladici vypis
  move++;

  int hashIndex, FunctNo;
  int retCode = OK;
  static int moveChain = 0;

  FunctNo = rand() % HTable->seedCnt;
  hashIndex = HashF(in.str, HTable->size, FunctNo);

  if(moveChain++ == 20){					//prestaveni tabulky po 20 (?) po sobe jdoucich move
    for(int i = 0; i < HTable->size; i++){			//prvek se ulozi do 1. volneho slotu a nasleduje prestaveni tabulky
      if(HTable->HT[i].str[0] == '\0'){				//neni-li zadny volny slot, vkladany prvek se zahodi
        HTable->HT[i] = in;
        break;
      }
    }

    fprintf(logF, "cycle detected, rebuild neccessary\n\n");		//ladici vypis
    return HTABLE_CYCLE;					//je potreba prestavet tabulku
  }


  if(HTable->HT[hashIndex].str[0] == '\0'){
    HTable->HT[hashIndex] = in;
    fprintf(logF, "move completed to %d ... random funct. no: %d\n", hashIndex, FunctNo);
    return OK;
  }
  else{
    HTableItem ToBMoved;

    ToBMoved = HTable->HT[hashIndex];
    HTable->HT[hashIndex] = in;
    retCode = HTableMoveRand(HTable, ToBMoved);
  }

  moveChain = 0;

  return retCode;
}
/*---------------------------------------------------------------------------------------------------------------------------------------*/

int HTableStore(HTABLE HTable, HTableItem in){

  fprintf(logF, "Calling HTableStore: %s...   ", in.str);		//ladici vypis
  store++;

  int *HFs;								//vysledky hash funkci

  if((HFs = malloc(HTable->seedCnt * sizeof(int))) == NULL){		//alokace pameti pro vysledky hash fci
    fprintf(stderr, "err: malloc error\n");
    exit(ERR);
  }

  for(int i = 0; i < HTable->seedCnt; i++){					//je prvek uz v tabulce?
    HFs[i] = HashF(in.str, HTable->size, HTable->seeds[i]);
    if(HTable->HT[HFs[i]].str[0] != '\0' && strcmp(HTable->HT[HFs[i]].str, in.str) == 0){
      free(HFs);
      fprintf(logF, "store canceled, string already in the table\n");		//ladici vypis
      return OK;								//jestli ano, nedela se nic
    }
  }

  for(int i = 0; i < HTable->seedCnt; i++){
    if(HTable->HT[HFs[i]].str[0] == '\0'){					// uloz do 1. volneho slotu
      HTable->HT[HFs[i]] = in;							//
      fprintf(logF, "store completed to %d,   funct no: %d\n", HFs[i], i);		//ladici vypis
      free(HFs);
      return OK;
    }
  }							//volny slot se nenasel, je potreba vyhodit jiny prvek

  HTableItem ToBMoved;					//promenna pro ulozeni vyhazovaneho prvku
  int HFpom;						//udkud byl prvek vyhozen

  ToBMoved = HTable->HT[HFs[0]];						//ulozi do 1. slotu a vola HTableMove
  HTable->HT[HFs[0]] = in;							//pro puvodni prvek
  HFpom = HFs[0];
  fprintf(logF, "store completed to %d,   funct no: 0, move necessary\n", HFs[0]);		//ladici vypis

  free(HFs);
  return HTableMove(HTable, ToBMoved, HFpom);

}
/*---------------------------------------------------------------------------------------------------------------------------------------*/

int HTableStoreRand(HTABLE HTable, HTableItem in){

  fprintf(logF, "Calling HTableStoreRand: %s...   ", in.str);		//ladici vypis
  store++;

  int *HFs;
  int FunctNo;							//vysledky hash funkci

  if((HFs = malloc(HTable->seedCnt * sizeof(int))) == NULL){		//alokace pameti pro vysledky hash fci
    fprintf(stderr, "err: malloc error\n");
    exit(ERR);
  }

  for(int i = 0; i < HTable->seedCnt; i++){					//je prvek uz v tabulce?
    HFs[i] = HashF(in.str, HTable->size, HTable->seeds[i]);
    if(HTable->HT[HFs[i]].str[0] != '\0' && strcmp(HTable->HT[HFs[i]].str, in.str) == 0){
      free(HFs);
      fprintf(logF, "store canceled, string already in the table\n");		//ladici vypis
      return OK;								//jestli ano, nedela se nic
    }
  }

  FunctNo = rand() % HTable->seedCnt;

  if(HTable->HT[HFs[FunctNo]].str[0] == '\0'){
    HTable->HT[HFs[FunctNo]] = in;
    fprintf(logF, "store completed to %d,   random funct no: %d\n", HFs[FunctNo], FunctNo);		//ladici vypis

    return OK;
  }

  HTableItem ToBMoved;
  ToBMoved = HTable->HT[HFs[FunctNo]];
  HTable->HT[HFs[FunctNo]] = in;
  fprintf(logF, "store completed to %d,   random funct no: %d, move neccessary\n", HFs[FunctNo], FunctNo);	//ladici vypis

  free(HFs);
  return HTableMoveRand(HTable, ToBMoved);
}
/*---------------------------------------------------------------------------------------------------------------------------------------*/

int HTableRemove(HTABLE HTable, HTableItem out){

  fprintf(logF, "Calling HTableRemove\n");		//ladici vypis

  int HF;

  for(int i = 0; i < HTable->seedCnt; i++){
    HF = HashF(out.str, HTable->size, HTable->seeds[i]);
    if(HTable->HT[HF].str[0] != '\0' && strcmp(HTable->HT[HF].str, out.str) == 0){
      HTable->HT[HF].str[0] = '\0';
      return OK;
    }
  }

  return OK;
}
/******************************************************           MAIN           *********************************************************/

int main(int argc, char *argv[]){

  int N;				//velikost hash tabulky
  int HASHFNM;				//pocet hash fci

  if(argc >= 2 && strcmp(argv[1], "-h") == 0) { PrintHelp(); exit(OK); }

  if(argc < 2) N = 3500;		//2. nepovinny argument - velikost hsh tabulky
  else N = atoi(argv[1]); 		//implicitne 3500

  if(argc < 3) HASHFNM = 4;		//3. nepovinny argument - pocet hash funkci
  else HASHFNM = atoi(argv[2]);		//implicitne 4

  if(argc < 4) inputF = fopen("in1", "r");	//4. nepovinny argument - vstupni soubor
  else inputF = fopen(argv[3], "r");			//implicitne in1

  if(HASHFNM < 2) exit(ERR);

  srand((unsigned int) time(NULL));	//inicializace funkce rand()

  logF = stderr;
  outF = fopen("out", "w");
 // resF = fopen("results", "a");

  HTableItem in;		//prvek, ktery se bude vkladat do tabulky

  HTABLE HTable;		//Tabulka

  if((HTable = malloc(sizeof(struct Htable))) == NULL){
    fprintf(stderr, "err: malloc error\n");
    exit(ERR);
  }

  /* vychozi hodnoty hashovaci tabulky  */
  HTable->size = N;
  HTable->seedCnt = HASHFNM;

  if((HTable->seeds = malloc(HTable->seedCnt * sizeof(int))) == NULL){
    fprintf(stderr, "err: malloc error\n");
    exit(ERR);
  }

  HTableGenerateSeeds(HTable);
  HTable->HT = NULL;
  if(HTableInit(HTable) != OK) exit(ERR);
  //logF = fopen("log.txt", "w");


  for(;;){

    if(fscanf(inputF, "%s ", in.str) == EOF) break;

/*----------------------------------------------------------------------------------------------*/
    fprintf(logF, "\ninput %d: %s ----", ++input, in.str);					//vypis vstupniho retezce
    for(int i = 0; i < HTable->seedCnt; i++)							//
    fprintf(logF, "HashF%d: %d ---- ", i, HashF(in.str, HTable->size, HTable->seeds[i]));	//a vyslednych indexu
    fprintf(logF, "\n");									//
/*----------------------------------------------------------------------------------------------*/

    if(HTableStore(HTable, in) != OK){			//zacykleni fce HTableStore => prestaveni HTable
      do{
      }while(HTableRebuild(HTable) != OK);
    }
  }

  HTablePrint(HTable);
//  StoreStats();
  HTableDrop(HTable);
  free(HTable);

  fclose(logF);
  fclose(inputF);
  fclose(outF);
 // fclose(resF);

  return OK;
}
