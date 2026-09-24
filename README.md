# EX-word Hardware Dump

日本語 | [English](#english)

CASIO EX-wordの非公開ハードウェア情報を、既知のレジスタと安全なメモリ確保試験から取得するhomebrew診断ツールです。XD-B4800実機では、SH7305系SH-4A、1コア、Iクロック24.184 MHz、Pクロック12.092 MHz、7.5 MiBの連続ヒープ確保を確認しました。

- APPID: `HWDMP`
- PVR、PRR、FRQCR、ROMヘッダーを読み取り
- RTCとTMU2からクロックを測定
- TMU2の元状態を測定後に復元
- 256 KiB単位で確保・端点書込・解放を検証
- 結果を本体内蔵領域へ保存

未知アドレスの総当たりやfirmware/RAMのダンプは行いません。確認済み機種はXD-B4800のみです。他機種での実行はレジスタ配置を確認してから行ってください。

GPL-2.0。ゲームデータ、CASIO firmware、端末認証情報は含みません。

ビルドには[`brain-hackers/exword-template`](https://github.com/brain-hackers/exword-template)、[`libdataplus`](https://github.com/brijohn/libdataplus)、devkitSH4を使用します。依存物は各上流repoから直接取得してください。関連プロジェクトは[`EXLLM`](https://github.com/ToTo-40417/exllm)、[`exllm-exword`](https://github.com/ToTo-40417/exllm-exword)、[`exword-gnuboy-save-importer`](https://github.com/ToTo-40417/exword-gnuboy-save-importer)です。

## English

EX-word Hardware Dump is a homebrew diagnostic tool that reads undocumented hardware information through known registers and bounded allocation tests. On an XD-B4800 it identified a single-core SH7305-family SH-4A, measured a 24.184 MHz instruction clock and 12.092 MHz peripheral clock, and verified a 7.5 MiB contiguous application heap.

- APPID: `HWDMP`
- Reads PVR, PRR, FRQCR, and the ROM header
- Measures clocks using the RTC and TMU2
- Restores the original TMU2 state after measurement
- Tests allocation in bounded 256 KiB steps
- Saves results to internal storage

It does not scan unknown addresses or dump firmware/RAM. Only the XD-B4800 has been verified. GPL-2.0; no game data, CASIO firmware, or authentication data is included. Obtain [`exword-template`](https://github.com/brain-hackers/exword-template), [`libdataplus`](https://github.com/brijohn/libdataplus), and devkitSH4 from their upstream sources. Related projects: [`EXLLM`](https://github.com/ToTo-40417/exllm), [`exllm-exword`](https://github.com/ToTo-40417/exllm-exword), and [`exword-gnuboy-save-importer`](https://github.com/ToTo-40417/exword-gnuboy-save-importer).
