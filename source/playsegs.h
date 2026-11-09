/* GERADO: trechos jogaveis (andar pelo mapa) do roteiro */
#ifndef PLAYSEGS_H
#define PLAYSEGS_H
#include "script.h"
static const PHot PH_0[] = {{3,-1.4,.6,1.2,"Esperar Mateus no portão",0,-1,-1,0,1,""}};
static const PNpc PN_0[] = {{1,"marcos",-3.4,-1.5,1.2,"idle",""},
  {3,"marcos",2.8,1,-1.5,"idle",""}};
static const Node PA_1_0[] = {
  SFXN(SF_DOOR),
  NAR("LUCAS bate de leve na porta do banheiro."),
  SAY("LUCAS", "Mateus ? Posso entrar ?"),
  SAY("MATEUS (de dentro)", "Entra"),
  END()
};
static const PHot PH_1[] = {{1,-2.4,-2.2,1.4,"Bater na porta do banheiro",PA_1_0,-1,-1,0,1,""}};
static const PNpc PN_1[] = {{3,"marcos",2.6,-1.5,-1.6,"idle",""}};
static const Node PA_2_0[] = {
  SAY("MATEUS (esforço)", "Carlos… merda… esse porta malas não abre"),
  MASH("Merda de porta malas !", 8),
  END()
};
static const PHot PH_2[] = {{7,5.8,-.2,1.5,"Forçar o porta-malas",PA_2_0,-1,-1,0,1,""}};
static const PNpc PN_2[] = {{7,"lucas",4.2,2.2,-1.2,"idle",""},
  {7,"marcos",5.4,2.1,-1.6,"idle",""}};
static const Node PA_3_0[] = {
  ART(ART_GELADEIRA),
  NAR("Dentro da sua geladeira vemos: um pote de ketchup. Um pote de mostarda. Um filé de frango. Duas garrafas de 2L de água. Um miojo. Uma camiseta."),
  CHOICE("Lucas olha para a geladeira.", "Pegar a camiseta e vestir", "Fechar a geladeira e ficar sem camisa"),
  ART(ART_NONE),
  NAR("LUCAS pega a camiseta de dentro da geladeira e veste."),
  END()
};
static const PHot PH_3[] = {{9,1.4,-1.4,1.5,"Abrir a geladeira",PA_3_0,-1,-1,0,1,""}};
static const PNpc PN_3[] = {{0,0,0,0,0,0,0}};
static const Node PA_4_0[] = {
  SFXN(SF_SPLASH),
  NAR("Ele abre a torneira e joga uma água no rosto, ele melhora um pouco, até reparar num percevejo morto em cima da pia."),
  END()
};
static const Node PA_4_1[] = {
  MUS(MUS_CHAOS),
  ART(ART_PERCEVEJO),
  NAR("MARCOS começa a olhar fixamente para ele. O percevejo está completamente destroçado, seu sangue amarelo e suas patas esmagadas ficaram espalhados ao redor da pia, e algumas formigas começam a ficar em cima e a entrar dentro do seu corpo."),
  NAR("Essa cena começa a deixar o MARCOS mal, seu coração começa a acelerar, e os seus ouvidos começam a zumbir."),
  SFXN(SF_HIT),
  ART(ART_NONE),
  NAR("TOC, TOC"),
  SAY("LUCAS", "Marcos, já acabou aí ? O Mateus preparou o corpo , agora só falta você lá em baixo"),
  END()
};
static const Node PA_4_2[] = {
  SFXN(SF_SPLASH),
  NAR("MARCOS não se aguenta, abre a tampa do vaso e vomita tudo lá dentro."),
  END()
};
static const PHot PH_4[] = {{5,1,-1.0,1.3,"Abrir a torneira",PA_4_0,0,1,0,0,"Olhe o percevejo na pia"},
  {5,1,-1.0,1.3,"Olhar o percevejo",PA_4_1,1,2,0,0,"Use o vaso"},
  {5,-1.5,-1.0,1.3,"Abrir a tampa do vaso",PA_4_2,2,-1,0,1,""}};
static const PNpc PN_4[] = {{0,0,0,0,0,0,0}};
static const PHot PH_5[] = {{2,3.0,-1.5,1.6,"Segurar o telefone",0,-1,-1,0,1,""}};
static const PNpc PN_5[] = {{2,"mateus",-3.2,1.4,-1.5,"idle",""},
  {2,"marcos",-2.4,.8,-1.6,"idle","axe"},
  {2,"carlos",0,-.2,1.57,"dead",""}};
static const PHot PH_6[] = {{3,1.2,1.6,1.7,"Golpear com o facão",0,-1,-1,0,1,""}};
static const PNpc PN_6[] = {{3,"mateus",1.2,1.6,-1.8,"aim","gun"}};
static const PHot PH_7[] = {{1,1.2,-.3,1.7,"Sentar no sofá",0,-1,-1,0,1,""}};
static const PNpc PN_7[] = {{3,"mateus",1.2,1.6,-1.8,"dead",""}};
static const Node PA_8_0[] = {
  SFXN(SF_DOOR),
  END()
};
static const PHot PH_8[] = {{1,-4.0,1.9,1.6,"Atender a porta",PA_8_0,-1,-1,0,1,""}};
static const PNpc PN_8[] = {{0,0,0,0,0,0,0}};
static const PHot PH_9[] = {{1,1.2,.7,1.5,"Ver quem está no sofá",0,-1,-1,1,0,"Vá até a garagem"},
  {3,.2,.4,1.8,"Examinar o corpo",0,-1,-1,1,1,""}};
static const PNpc PN_9[] = {{3,"mateus",.2,.4,1.0,"dead",""}};
static const PSeg PSEG[] = {
{"lucas","",1,0,0,0,"Vá até a garagem esperar o portão (porta à esquerda)",122,PH_0,1,PN_0,2},
{"lucas","",3,0,0,0,"Vá até a porta do banheiro e fale com o Mateus",90,PH_1,1,PN_1,1},
{"mateus","",7,0,0,0,"Abra o porta-malas do carro (ESPAÇO repetidamente)",128,PH_2,1,PN_2,2},
{"lucasS","",9,0,0,0,"Guarde o sorvete: abra a geladeira",512,PH_3,1,PN_3,0},
{"marcos","",5,1,-0.2,0.6,"Marcos: use a pia para se acalmar",122,PH_4,3,PN_4,0},
{"lucas","",2,0,0,0,"Dê passos para trás até o telefone vermelho",4,PH_5,1,PN_5,3},
{"lucasR","chainsaw",3,0,0,0,"Ataque o Mateus!",8,PH_6,1,PN_6,1},
{"lucasB","",3,0,0,0,"Vá mancando até a sala e sente no sofá",10,PH_7,1,PN_7,1},
{"lucas","",1,1,-1.5,0.8,"Atenda a porta antes que ele desconfie",2,PH_8,1,PN_8,0},
{"tio","",1,0,0,0,"Procure por alguém na casa",26,PH_9,2,PN_9,1},
};
#endif
