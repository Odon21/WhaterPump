#ifndef HTML_H
#define HTML_H
const char config_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8">
    <title>Controle de Bomba</title>
    <meta name="viewport" content="width=device-width" />
    <style>
      body {
        font-family: 'Segoe UI', Arial, sans-serif;
        /*font-family: Arial;*/
        text-align: center;
        background: #CFCFCF;
      }
      h2 {
        color: #333;
      }
      form {
        border: 1px solid rgba(0, 0, 0, 0.596);
        background: #f2f2f2;
        padding: 20px;
        display: inline-block;
        border-radius: 10px;
        box-shadow: 0 0 10px #ccc;
      }
      input {
        margin: 5px;
        padding: 8px;
      }
      button {
        background: #2196F3;
        color: white;
        border: none;
        padding: 10px 20px;
        border-radius: 5px;
        cursor: pointer;
      }
      button:hover {
        background: #1976D2;
      }
      #s_wifi{
        color: red;
      }
      #timeForm{
        margin: 0px 50x 10px 50px;
        border: 1px solid rgba(0, 0, 0, 0.596);
        background: #f2f2f2;
      }
    </style>
  </head>
  <body>
    <h2>Configuração de Horário</h2>
    <h4>Insira o tempo de operação da bomba</h4>
    <form id="form">
      <h3>1º Horário</h3>
      <label>Ligar as</label><br>
      <input type="time" name="start1" value="value_start1"><br>
      <label>Desligar as</label><br>
      <input type="time" name="stop1" value="value_stop1"><br><br>
      <h3>2º Horário</h3>
      <label>Ligar as:</label><br>
      <input type="time" name="start2" value="value_start2"><br>
      <label>Desligar as:</label><br>
      <input type="time" name="stop2" value="value_stop2"><br><br>
      <h3>WiFi</h3>
      <label id="s_wifi" style="font-size: 0.7rem; margin-bottom: 5px; margin-top: -10px;">Password de WiFi (máx 10 caracteres)</label><br>
      <input type="text"  name="wifiPass" maxlength="10" required="" value="value_wifiPass"><br><br>
      <button type="submit">Salvar</button>
    </form>
    <br>
    <hr>
    <h3>Ajustar Data e Hora</h3>
    <form id='timeForm'> 
      Data <input type='date' id='date' required><br>
      Hora <input type='time' id='time' step='1' required><br>
      <button type='button' style="margin-top: 7px" onclick='setTime()'>Definir</button>
    </form>
    <p id='status'></p>
    <p id='dev'></p>
    <div style="text-align:right; margin-top:24px; color:#888; font-size:0.7rem;">
      <a href="mailto:odoncandidotda@gmail.com? subjet=subjet text">Enviar E-mail ao fabricante</a><br> Contacto: 865433683
    </div>
    <script>
    function setTime() {
      const date = document.getElementById('date').value;
      const time = document.getElementById('time').value;
      const statusEl = document.getElementById('status');

      if (!date || !time) {
        statusEl.innerText = '⚠️ Por favor preencha todos os campos';
        return;
      }
      const xhr = new XMLHttpRequest();
      xhr.open('POST', '/set_time', true);
      xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
      xhr.onreadystatechange = function() {
        if (xhr.readyState === 4) {
          if (xhr.status === 200) {
            statusEl.innerText = '✅ Data e hora definida.';
            alert('✅ Acerto de data e hora realizado com sucesso!');
          } else {
            statusEl.innerText = '❌ Erro ao definir hora';
            alert('❌ Erro ao definir hora');
          }
        }
      };
      xhr.send('datetime=' + encodeURIComponent(date + 'T' + time));
      statusEl.innerText = "📤 Definindo data e hora...";
    };
    document.getElementById("form").onsubmit = async (e) => {
      e.preventDefault();
      const data = new FormData(e.target);
      const resp = await fetch("/set", { method: "POST", body: data });
      const msg = await resp.text();
      console.log(msg); 
      alert(msg.replace(/<br>/g, "\n").replace(/<b>|<\/b>/g, ""));
    };
    </script>
  </body>
</html>
)rawliteral";
#endif
