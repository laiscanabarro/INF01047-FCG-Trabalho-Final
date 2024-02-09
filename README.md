# INF01047 - Fundamentos de Computação Gráfica (FCG) - Trabalho Final
Repositório para o Trabalho Final da disciplina de Fundamentos de Computação Gráfica.

Alunos: 
Laís Fernanda Canabarro Araujo

Leonardo Dalpian Dutra

Ideia inical: Planejamos desenvolver uma aplicação gráfica interativa de exploração espacial pelo Sistema Solar. A proposta consiste em criar modelos realistas para os planetas, luas e outros corpos celestes, permitindo aos usuários explorar o espaço de forma intuitiva. 

### Contribuição de cada membro da dupla para o trabalho:

Laís implementou os seguintes requisitos:

  - Objetos da cena virtual (sphere, spaceship, rock, asteroid);
  - Mapeamento de texturas (13 images) em todos os objetos;
  - Transformações geométrica controladas pelo usuário: movimentação da cena pelo teclado;
  - Animações baseadas no tempo: translação e rotação dos planetas e movimentação da nave junto com a câmera;
  - Movimentação com curva Bézier cúbica para os objeto "Asteroid" e "Spaceship".

Leonardo implementou os seguintes requisitos:
  - Skybox;
  - Modelos de iluminação de objetos geométricos;

### Uso do ChatGPT: 
Foi utlizado para auxílio em questões pontuais do código e como base para o desenvolvimento da função da curva de Bezier.

 para desenvolvimento do trabalho, descrevendo como a ferramenta foi utilizada e para quais partes do trabalho. O parágrafo deve também incluir uma análise crítica descrevendo quão útil a dupla achou a ferramenta, onde ela auxiliou e onde ela não auxiliou adequadamente;

### Descrição do processo de desenvolvimento e do uso dos conceitos de Computação Gráfica:
O trabalho começou seu desenvolvimento a partir do código fonte do Laboratório 4. Além disso, o Laboratório 5 também serviu de auxílio para o mapeamento de texturas e do Laboratório 2 foi utilizado para implementar a movimentação da câmera a partir do teclado.

Em  geral, os passos desenvolvidos, em sequência, foram:
  - criação do cenário (com Sol e Mercúrio);
  - movimentação da câmera com o teclado;
  - animações dos planetas;
  - implementação do restante dos planetas;
  - implementação da Skybox;
  - implementação de novos objetos;
  - curva de Bezier.

### Funcionamento da aplicação:
![Exemplo]([https://example.com/image.png](https://github.com/laiscanabarro/INF01047-FCG-Trabalho-Final/blob/main/imagens/cap1.png))
![Exemplo]([https://example.com/image.png](https://github.com/laiscanabarro/INF01047-FCG-Trabalho-Final/blob/main/imagens/cap2.png))
    
### Manual:
- Teclado:
  - Teclas W, A, S, D: movimentação da câmera para frente (W), esquerda (A), trás (S), direita (D);
  - Teclas "ESPAÇO" e Shift: movimentação da câmera para cima ("ESPAÇO") e para baixo (Shift);

- Mouse:
  -  Botão esquerdo: movimentação da câmera;

### Passo a passo para execução do programa:
- Abra o arquivo Laboratorio_5.cbp no diretório "INF01047-FCG-Trabalho-Final" com o programa CodeBlocks;
- Compile e execute o código em Build > Build and Run;
  
  

- Um vídeo de 3 a 5 minutos com uma apresentação do trabalho feito pela dupla, incluindo uma demonstração da aplicação gráfica. Para gravar seu vídeo, você pode usar o software OBS Studio. Preferencialmente, você deve incluir um link para o vídeo no Youtube dentro do seu ZIP (vídeo público ou "unlisted"). Se você não quiser usar o Youtube por algum motivo, pode incluir o vídeo no formato MP4 dentro do ZIP.
