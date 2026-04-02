# **Step :** <br />
cmake -S . -B build -G Ninja -DWOUOUI_SKETCH=../src/main.ino <br />
export PATH=/mingw64/bin:$PATH <br />
cd build && ninja <br />
./Emulator.exe <br />


# **Remove build :** <br />
rm -rf build


# **Chronos Bridge (Emulator) :** <br />
Emulator sekarang punya input bridge UDP lokal untuk data Bluetooth/Navigasi. <br />
Port: `127.0.0.1:9233` (UDP) <br />

Format per baris (dipisah `|`): <br />
- `CONN|1` atau `CONN|0` <br />
- `TIME|1` <br />
- `PBAT|87|0` (level|charging) <br />
- `NAV|1|Title|Directions|Distance|ETA|Duration` <br />
- `NAVICON|pos|CRCHEX|HEXDATA` (icon 48x48, dikirim per chunk 96 byte) <br />
- `NAVICONFULL|CRCHEX|HEXDATA` (icon penuh 288 byte, format utama) <br />
- `NOTIF|38|App|Title|Message|12:34` <br />

Contoh cepat dari PowerShell: <br />
`$udp = New-Object System.Net.Sockets.UdpClient; $ep = New-Object System.Net.IPEndPoint([System.Net.IPAddress]::Parse("127.0.0.1"),9233); $msg = [Text.Encoding]::UTF8.GetBytes("CONN|1`nNAV|1|Office|Turn right|120 m|12:34|2 min"); [void]$udp.Send($msg,$msg.Length,$ep); $udp.Close()` <br />


# **Chronos App via Bluetooth PC (Bridge BLE) :** <br />
Untuk konek langsung dari aplikasi Chronos di HP ke emulator, jalankan bridge BLE di PC: <br />
`dotnet run --project ../tools/ChronosBleBridge/ChronosBleBridge.csproj -c Release` <br />

Urutan pakai: <br />
1. Jalankan `Emulator.exe` <br />
2. Aktifkan `Bluetooth ON` di menu emulator <br />
3. Jalankan bridge BLE command di atas <br />
4. Pair/connect dari aplikasi Chronos di HP <br />
5. Data notif/nav dari Chronos akan masuk ke emulator via UDP bridge <br />
Nama BLE yang di-broadcast oleh bridge: `DASAI Bridge` <br />

Jika bridge menampilkan `RadioNotAvailable`, aktifkan Bluetooth di Windows atau gunakan adapter yang mendukung BLE peripheral role. <br />
Catatan: render icon navigation pada emulator memakai direct update (tanpa swap animation) agar tidak muncul artefak ghost saat stream BLE. <br />
