function doGet(e) {
  var ss = SpreadsheetApp.getActiveSpreadsheet();
  var sheet = ss.getSheets()[0]; // Menulis di sheet pertama (paling kiri)
  var waktuSekarang = new Date();
  
  // Menangkap data yang dikirim dari HTML Dashboard via browser
  var barang = e.parameter.barang;
  var logam = e.parameter.logam;
  var nonlogam = e.parameter.nonlogam;
  
  // Otomatis ketik di baris paling bawah: Waktu, Total, Logam, Non-Logam
  sheet.appendRow([waktuSekarang, barang, logam, nonlogam]);
  
  // === KIRIM DATA REAL-TIME KE GRUP TELEGRAM LU ===
  kirimKeTelegramGrup(barang, logam, nonlogam);
  
  // Memberikan respon balik ke browser kalau data sukses masuk
  return ContentService.createTextOutput("Sukses Masuk Sheets & Telegram!")
                       .setMimeType(ContentService.MimeType.TEXT);
}

function kirimKeTelegramGrup(barang, logam, nonlogam) {
  // Token dan Chat ID grup lu sudah gua masukin langsung di bawah ini
  var tokenBot = "8776650277:AAHTJME8G2XczeFizZPv1ZajSZHZL5I8MdQ"; 
  var chatIdGrup = "-1003538983739"; 
  
  // Format teks laporan rapi dengan emoji ala industri
  var isiPesan = "🤖 *SMART CONVEYOR REPORT* 🤖\n" +
                 "=========================\n" +
                 "🕒 *Waktu* : " + Utilities.formatDate(new Date(), "GMT+7", "dd-MM-yyyy HH:mm:ss") + " WIB\n" +
                 "📦 *Total Barang* : " + barang + " pcs\n" +
                 "⚙️ *Barang Logam* : " + logam + " pcs\n" +
                 "🍂 *Non-Logam* : " + nonlogam + " pcs\n" +
                 "=========================\n" +
                 "✅ _Data sinkron dengan Dashboard HTML._";
                 
  var urlTelegram = "https://api.telegram.org/bot" + tokenBot + "/sendMessage";
  
  var payload = {
    "method": "post",
    "contentType": "application/json",
    "payload": JSON.stringify({
      "chat_id": chatIdGrup,
      "text": isiPesan,
      "parse_mode": "Markdown"
    })
  };
  
  // Kirim data langsung dari server Google ke Telegram grup lu
  UrlFetchApp.fetch(urlTelegram, payload);
}