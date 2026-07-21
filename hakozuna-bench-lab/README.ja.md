# Hakozuna Bench Lab

Hakozuna Bench Labは、memory allocatorのベンチマークを実行・比較・可視化
するWindows-firstのデスクトップアプリです。

既存のベンチ実行ファイルを作り直すのではなく、再現可能なGUI入口を与え、
throughput、RSS、latency、安全性の結果を同じ形式で比較できるようにします。

## 最初のリリース

```text
platform: Windows 10/11 x64 first; macOS next
UI: .NET 8 + Avalonia UI
execution: 隔離した子プロセス
allocators: system, HZ8, mimalloc, tcmalloc
presets: local, remote-heavy, mixed-size, RSS turnover
exports: JSON, CSV, Markdown
```

最初はWindows版を主対象にし、次にmacOS版を追加します。Linux対応はWindowsと
macOSのworkflow、結果protocolが固まった後に追加し、MVPを遅らせません。

## 最重要ルール

GUI自身のprocessへallocator DLLを読み込みません。allocatorは準備済みの
benchmark executableまたはinterpose用の子processで動かします。DLLやallocator
がcrashしても、結果を見るGUIまで巻き込まない境界を維持します。

## 測定モード

| Mode | 目的 | GUIの動き | 結果の扱い |
|---|---|---|---|
| Preview | 手早い探索 | 進捗と概算RSSをライブ表示 | 方向確認のみ |
| Verified | 比較可能な測定 | 高頻度pollingを止め、終了後に描画 | レポート利用可能 |

Verified modeでは、実行ファイルhash、allocator名、引数、machine情報、実行順、
終了code、raw stdout/stderrを保存します。必要runが失敗した場合やallocatorの
身元が曖昧な場合は、正式結果として扱いません。

## ドキュメント

- [製品charter](docs/CHARTER.md)
- [architecture](docs/ARCHITECTURE.md)
- [Windows MVP](docs/MVP_WINDOWS.md)
- [result protocol](docs/RESULT_PROTOCOL.md)
- [current task](current_task.md)

## MVPでやらないこと

- GUIからallocator sourceを編集する
- third-party allocator binaryを無断downloadする
- Windows/Linuxの性能が同じだと主張する
- diagnostic counter入りbuildを速度結果として表示する
- GUI process内で未信頼DLLを読み込む
- 論文向けcommand-line runnerを廃止する

製品promiseは単純です。allocatorとworkloadを選び、同じ条件で実行し、
PowerShell scriptを読まなくても速度とRSSのtradeoffを理解できるようにします。
