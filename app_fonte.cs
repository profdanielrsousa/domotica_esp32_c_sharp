/* PROJETO FINAL CURSO CPS (PROGRAMAÇÃO C# COM ESP32)
 * SENSOR IOT DE PRESENÇA E GÁS INTELIGENTE
 * 
 * ESP32 NODEMCU - DEVKIT V1 (30 PINOS)
 * SENSOR DE GÁS (GLP E GÁS NATURAL) MQ-05
 * SENSOR DE UMIDADE RELATIVA E TEMPERATURA DHT11
 * SENSOR DE PRESENÇA PIR HC-SR501
 * BUZZER ATIVO
 * MÓDULO RELÉ (LED)
 * 
 * AUTOR: DANIEL RODRIGUES DE SOUSA 10/09/2024 */

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Net.WebSockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace Projeto_Final
{
    public partial class Form1 : Form
    {
        ClientWebSocket webSocket; // Declaração do objeto WebSocket para comunicação com o servidor

        private System.Windows.Forms.Timer timer; // Declaração do temporizador
        private bool toggleColor; // Flag para alternar a cor
        private int sm_mensagem = 0; // Variável para controlar o estado das mensagens

        private async Task ReceiveMessages() // Método assíncrono para receber mensagens do WebSocket
        {
            var buffer = new byte[1024]; // Cria um buffer de bytes para armazenar os dados recebidos
            while (webSocket.State == WebSocketState.Open) // Loop enquanto a conexão WebSocket estiver aberta
            {
                try
                {
                    var result = await webSocket.ReceiveAsync(new ArraySegment<byte>(buffer), CancellationToken.None); // Recebe uma mensagem do WebSocket de forma assíncrona
                    var message = Encoding.UTF8.GetString(buffer, 0, result.Count); // Converte os bytes recebidos em uma string usando UTF-8

                    if (message == "Vazando gas!") // Verifica se a mensagem indica vazamento de gás
                    {
                        label4.Invoke((MethodInvoker)(() => label4.Text = "Vazando gas!")); // Atualiza o label4 para mostrar "Vazando gas!" na UI
                        label4.Font = new Font("Arial", 18); // Define o tamanho da fonte
                        label4.ForeColor = Color.Red; // Define a cor do texto como vermelho
                    }
                    else if (message == "Gas ok!") // Verifica se a mensagem indica que o gás está ok
                    {
                        label4.Invoke((MethodInvoker)(() => label4.Text = "Gas ok!")); // Atualiza o label4 para mostrar "Gas ok!" na UI
                        label4.Font = new Font("Arial", 18); // Define o tamanho da fonte
                        label4.ForeColor = Color.Blue; // Define a cor do texto como azul
                    }

                    int num_sensor;
                    // Tenta converter a string para um número inteiro 
                    bool ehNumeroInteiro = int.TryParse(message.ToString(), out num_sensor);

                    // Se for número, atualiza os componentes da UI de acordo com o tipo de mensagem
                    if (ehNumeroInteiro)
                    {
                        switch (sm_mensagem)
                        {
                            case 0:
                                this.Invoke(new MethodInvoker(delegate
                                {
                                    num_sensor = Convert.ToInt32(message);
                                    if ((num_sensor >= 0) && (num_sensor <= 1))
                                    {
                                        progressBar4.Value = num_sensor; // Atualiza o valor da progressBar4
                                    }
                                }));
                                break;

                            case 1:
                                num_sensor = Convert.ToInt32(message);
                                if ((num_sensor >= 0) && (num_sensor <= 50))
                                {
                                    this.Invoke(new MethodInvoker(delegate
                                    {
                                        progressBar1.Value = Convert.ToInt32(message); // Atualiza o valor da progressBar1
                                    }));
                                    label5.Invoke((MethodInvoker)(() => label5.Text = $"Temperatura ({message} °C)")); // Atualiza o label5 para mostrar a temperatura
                                }
                                break;

                            case 2:
                                num_sensor = Convert.ToInt32(message);
                                if ((num_sensor >= 0) && (num_sensor <= 100))
                                {
                                    this.Invoke(new MethodInvoker(delegate
                                    {
                                        progressBar2.Value = Convert.ToInt32(message); // Atualiza o valor da progressBar2
                                    }));
                                    label6.Invoke((MethodInvoker)(() => label6.Text = $"Umidade ({message} %)")); // Atualiza o label6 para mostrar a umidade
                                }
                                break;

                            case 3:
                                num_sensor = Convert.ToInt32(message);
                                if ((num_sensor >= 0) && (num_sensor <= 1))
                                {
                                    progressBar3.Value = num_sensor; // Atualiza o valor da progressBar3
                                }
                                break;
                        }
                    }

                    textBox1.Invoke((MethodInvoker)(() => textBox1.AppendText($"Recebido: {message} \r\n"))); // Atualiza textBox1 na UI para mostrar a mensagem recebida

                }
                catch (Exception ex) // Bloco de captura para exceções (erros)
                {
                    label1.Invoke((MethodInvoker)(() => label1.Text = "Desconectado")); // Atualiza o label1 para mostrar "Desconectado" na UI
                    MessageBox.Show("Erro ao receber mensagem: " + ex.Message); // Exibe uma mensagem de erro ao usuário
                }
            }
        }

        public Form1()
        {
            InitializeComponent();
            timer = new System.Windows.Forms.Timer(); // Inicializa o temporizador
            timer.Interval = 500; // Define o intervalo do temporizador para 500 ms
            timer.Tick += async (sender, e) => await timer1_Tick(sender, e); // Adiciona o evento Tick ao temporizador
            timer.Start(); // Inicia o temporizador
        }

        private async void button1_Click(object sender, EventArgs e) // Método para tratar o clique do botão 1
        {
            webSocket = new ClientWebSocket(); // Inicializa uma nova instância de ClientWebSocket
            Uri serverUri = new Uri("ws://192.168.0.157:81"); // Define o URI do servidor WebSocket, substitua pelo IP do seu ESP32
            try
            {
                await webSocket.ConnectAsync(serverUri, CancellationToken.None); // Tenta conectar ao servidor WebSocket de forma assíncrona

                label1.Invoke((MethodInvoker)(() => label1.Text = "Conectado")); // Atualiza o label1 para mostrar "Conectado" na UI

                var message = "RELE_off"; // Define a mensagem "RELE_off"
                var bytes = Encoding.UTF8.GetBytes(message); // Converte a mensagem em um array de bytes usando UTF-8
                await webSocket.SendAsync(new ArraySegment<byte>(bytes), WebSocketMessageType.Text, true, CancellationToken.None); // Envia a mensagem via WebSocket de forma assíncrona

                message = "AUTOMATICO_on"; // Define a mensagem "AUTOMATICO_on"
                bytes = Encoding.UTF8.GetBytes(message); // Converte a mensagem em um array de bytes usando UTF-8
                await webSocket.SendAsync(new ArraySegment<byte>(bytes), WebSocketMessageType.Text, true, CancellationToken.None); // Envia a mensagem via WebSocket de forma assíncrona

                label2.Invoke((MethodInvoker)(() => label2.Text = "Modo automático: ON")); // Atualiza o label2 para mostrar "Modo automático: ON" na UI
                label3.Invoke((MethodInvoker)(() => label3.Text = "Liga lâmpada: OFF")); // Atualiza o label3 para mostrar "Liga lâmpada: OFF" na UI

                await ReceiveMessages(); // Chama o método para receber mensagens do servidor WebSocket
            }
            catch (Exception ex) // Bloco de captura para exceções
            {
                label1.Invoke((MethodInvoker)(() => label1.Text = "Desconectado")); // Atualiza o label1 para mostrar "Desconectado" na UI
                MessageBox.Show("Erro ao conectar: " + ex.Message); // Exibe uma mensagem de erro ao usuário
            }
        }

        private async void button2_Click(object sender, EventArgs e) // Método para tratar o clique do botão 2
        {
            if (label1.Text == "Conectado")
            {
                var message = "RELE_off"; // Define a mensagem "RELE_off"
                var bytes = Encoding.UTF8.GetBytes(message); // Converte a mensagem em um array de bytes usando UTF-8
                await webSocket.SendAsync(new ArraySegment<byte>(bytes), WebSocketMessageType.Text, true, CancellationToken.None); // Envia a mensagem via WebSocket de forma assíncrona

                message = "AUTOMATICO_on"; // Define a mensagem "AUTOMATICO_on"
                bytes = Encoding.UTF8.GetBytes(message); // Converte a mensagem em um array de bytes usando UTF-8
                await webSocket.SendAsync(new ArraySegment<byte>(bytes), WebSocketMessageType.Text, true, CancellationToken.None); // Envia a mensagem via WebSocket de forma assíncrona

                label2.Invoke((MethodInvoker)(() => label2.Text = "Modo automático: ON")); // Atualiza o label2 para mostrar "Modo automático: ON" na UI
                label3.Invoke((MethodInvoker)(() => label3.Text = "Liga lâmpada: OFF")); // Atualiza o label3 para mostrar "Liga lâmpada: OFF" na UI   
            }
        }

        private async void button3_Click(object sender, EventArgs e) // Método para tratar o clique do botão 3
        {
            if (label1.Text == "Conectado")
            {
                var message = "AUTOMATICO_off"; // Define a mensagem "AUTOMATICO_off"
                var bytes = Encoding.UTF8.GetBytes(message); // Converte a mensagem em um array de bytes usando UTF-8
                await webSocket.SendAsync(new ArraySegment<byte>(bytes), WebSocketMessageType.Text, true, CancellationToken.None); // Envia a mensagem via WebSocket de forma assíncrona
                label2.Invoke((MethodInvoker)(() => label2.Text = "Modo automático: OFF")); // Atualiza o label2 para mostrar "Modo automático: OFF" na UI
            }
        }

        private async void button4_Click(object sender, EventArgs e) // Método para tratar o clique do botão 4
        {
            if ((label2.Text == "Modo automático: OFF") && (label1.Text == "Conectado"))
            {
                var message = "RELE_on"; // Define a mensagem "RELE_on"
                var bytes = Encoding.UTF8.GetBytes(message); // Converte a mensagem em um array de bytes usando UTF-8
                await webSocket.SendAsync(new ArraySegment<byte>(bytes), WebSocketMessageType.Text, true, CancellationToken.None); // Envia a mensagem via WebSocket de forma assíncrona
                label3.Invoke((MethodInvoker)(() => label3.Text = "Liga lâmpada: ON")); // Atualiza o label3 para mostrar "Liga lâmpada: ON" na UI   
            }
        }
        private async void button5_Click(object sender, EventArgs e) // Método para tratar o clique do botão 5
        {
            if ((label2.Text == "Modo automático: OFF") && (label1.Text == "Conectado"))
            {
                var message = "RELE_off"; // Define a mensagem "RELE_off"
                var bytes = Encoding.UTF8.GetBytes(message); // Converte a mensagem em um array de bytes usando UTF-8
                await webSocket.SendAsync(new ArraySegment<byte>(bytes), WebSocketMessageType.Text, true, CancellationToken.None); // Envia a mensagem via WebSocket de forma assíncrona
                label3.Invoke((MethodInvoker)(() => label3.Text = "Liga lâmpada: OFF")); // Atualiza o label3 para mostrar "Liga lâmpada: OFF" na UI    
            }
        }

        private async Task timer1_Tick(object sender, EventArgs e) // Método para tratar o evento Tick do temporizador
        {
            var message = ""; // Define a mensagem como uma string vazia
            byte[] bytes = new byte[0]; // Inicializa como um array de bytes vazio

            if (label1.Text == "Conectado")
            {
                switch (sm_mensagem)
                {
                    case 0:
                        message = "TEMPERATURA"; // Define a mensagem "TEMPERATURA"
                        sm_mensagem = 1;
                        break;

                    case 1:
                        message = "UMIDADE"; // Define a mensagem "UMIDADE"
                        sm_mensagem = 2;
                        break;

                    case 2:
                        message = "PIR"; // Define a mensagem "PIR"
                        sm_mensagem = 3;
                        break;

                    case 3:
                        message = "RELE"; // Define a mensagem "RELE"
                        sm_mensagem = 0;
                        break;
                }
                bytes = Encoding.UTF8.GetBytes(message); // Converte a mensagem em um array de bytes usando UTF-8
                await webSocket.SendAsync(new ArraySegment<byte>(bytes), WebSocketMessageType.Text, true, CancellationToken.None); // Envia a mensagem via WebSocket de forma assíncrona	
                textBox1.Invoke((MethodInvoker)(() => textBox1.AppendText($"Enviado: {message} \r\n"))); // Atualiza textBox1 na UI para mostrar a mensagem enviada
            }
        }
    }
}
