# Monitor de Acesso de Laboratório

Este projeto implementa um sistema embarcado para controle de acesso a um laboratório, limitando a ocupação a 11 pessoas (10 alunos e 1 professor).
Utilizando o microcontrolador Raspberry Pi Pico e o sistema operacional em tempo real FreeRTOS, o sistema gerencia entradas e saídas de usuários, fornece feedback visual e sonoro, e permite o reset da contagem de ocupação.

## Funcionalidades

- **Controle de Ocupação:** Gerencia a entrada e saída de usuários, garantindo que o número de ocupantes não exceda o limite estabelecido.
- **Indicação Visual:** Utiliza um LED RGB para indicar o estado atual do laboratório:
  - Azul: Laboratório vazio
  - Verde: Ocupação normal
  - Amarelo: Última vaga disponível
  - Vermelho: Laboratório lotado
- **Feedback Sonoro:** Emite um beep quando o laboratório atinge a capacidade máxima e um beep duplo ao resetar a contagem.
- **Display OLED:** Exibe informações sobre o número de usuários e o estado atual do laboratório.
- **Botão de Reset:** Permite resetar a contagem de usuários, simulando a saída de todos os ocupantes.

## Hardware Utilizado

- **Microcontrolador:** Raspberry Pi Pico
- **Display:** OLED SSD1306 via I2C
- **LED RGB:** Para indicação de status
- **Buzzer:** Para feedback sonoro
- **Botões:** Três botões para entrada, saída e reset

## Software

- **Linguagem de Programação:** C
- **Sistema Operacional:** FreeRTOS
- **Bibliotecas:** SSD1306 para controle do display OLED

## Estrutura do Projeto

- `LabAcess.c`: Código principal do sistema.
- `lib/ssd1306.h`: Biblioteca para controle do display OLED.
- `FreeRTOSConfig.h`: Arquivo de configuração do FreeRTOS.
- `CMakeLists.txt`: Configuração do projeto para compilação.

## Como Compilar

1. Certifique-se de ter o SDK do Raspberry Pi Pico e o FreeRTOS configurados corretamente.
2. Clone este repositório:
   ```bash
   git clone https://github.com/DanielPortoBraz/MonitorDeAcessoDeLaboratorio.git
   ```
3. Navegue até o diretório do projeto:
   ```bash
   cd MonitorDeAcessoDeLaboratorio
   ```
4. Crie um diretório de build e compile o projeto:
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```
5. O arquivo `.uf2` gerado pode ser carregado no Raspberry Pi Pico.

---
## Vídeo de demonstração:
https://youtube.com/playlist?list=PLaN_cHSVjBi8bVXQ77KFUaqUE1xrwfi_7&si=5XDVtsXpv3InDWz4

---

## Licença

Este projeto está licenciado sob a [MIT License](LICENSE).

## Autor

- Daniel Porto Braz

Para mais detalhes, consulte o código-fonte disponível no repositório.

