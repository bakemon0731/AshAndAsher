# AshAndAsher（アッシュアンドアッシャー）

![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.6-313131?logo=unrealengine)
![C++](https://img.shields.io/badge/C%2B%2B-Language-00599C?logo=cplusplus)
![Status](https://img.shields.io/badge/Status-In%20Development-yellow)

<!-- 開発中の画面でもかまいませんので、現在動いているゲームのGIFアニメーションかスクリーンショットをここに貼ってください。
     動く画面が一つもない状態での公開は評価者の離脱リスクが高いため、公開前に必ず1点は用意することを推奨します。 -->

## 目次
- [概要](#概要)
- [ゲームのプレイについて](#-ゲームのプレイについて-作業中)
- [技術的なアピールポイント](#-技術的なアピールポイント)
- [評価していただきたい主要なクラス](#-評価していただきたい主要なクラス)
- [開発を通じて得た知見・今後の展望](#開発を通じて得た知見今後の展望)
- [ライセンス](#ライセンス)

## 概要

### ⚔️ ゲームのコンセプト
- **純粋な魔法詠唱バトル:** 近接武器に頼らず、記憶容量と装備でビルドした魔法のみでダンジョン攻略・他プレイヤーとの戦闘を行うPVPVEゲーム！
- **GASによる奥深い戦闘:** 装備によるステータスバフ、数多の魔法によるビルドへの選択肢で自分だけの戦闘！！

| 項目 | 内容 |
|---|---|
| 開発環境 | Unreal Engine 5.6 / Visual Studio 2022 / JetBrains Rider |
| 使用言語 | C++ / ブループリント |
| 制作人数 | 1人（個人制作） |
| 制作期間 | 2026年9月〜現在 |

## 🎮 ゲームのプレイについて (作業中)

現在開発中です。完了次第、以下のリンクにて実行ファイル（.exe）を公開予定です。

- ダウンロードリンク: 準備中（完了時にGoogle Drive等のURLを記載）
- プレイ動画（YouTube等）: 準備中

> ⚠️ **注記**  
> 本リポジトリにはテクスチャやモデルなどのバイナリアセットが含まれていないため、クローンしてUnrealエディタで開いてもゲームとしては動作しません。ゲームの挙動については、上記のプレイ動画をご参照ください。

## 技術的なアピールポイント（C++実装における設計の工夫）

### 1. GAS（Gameplay Ability System）を用いたスケーラブルな能力・属性管理

ゲーム規模の拡大を見据え、キャラクターのスキルやステータス管理にUE標準のフレームワークであるGASを導入しています。

- **Fatクラスの回避**: ダメージ処理やスキル発動ロジックをキャラクタークラスに直接書くのではなく、`GameplayAbility`としてカプセル化することで、キャラクタークラスの肥大化を防ぎます。
- **プロパティ管理**: `CombatAttributeSet` などを定義し、HPや攻撃力などのステータスを一貫したルールのもとで安全に管理・運用できる仕組みを構築しました。

### 2. サブシステムを使用したオブジェクトプール

頻繁なアクタ生成・破棄（`SpawnActor` / `Destroy`）に伴うCPU負荷やガベージコレクション（GC）スパイクを軽減するため、ワールド単位で機能する汎用オブジェクトプールをC++で構築しました。

- **クラス単位の汎用管理**: `TSubclassOf` をキーとしたマップ管理により、発射物（Projectile）だけでなくヒットエフェクトなど任意のワンショットActorを使い回せる拡張性を確保しています。
- **`IPoolableActor` インターフェースによる状態初期化**: プール取り出し時（`OnActivatedFromPool`）および返却時（`OnReturnToPool`）の専用フックを用意し、Transform更新やコリジョン・Tickの切り替えだけでなく、アクタ固有の状態リセットや演出の後始末を安全に行える仕組みを構築しました。
- **`PrewarmPool`（事前生成）によるスパイク防止**: レベルロード時などに指定数のアクタをあらかじめ生成してプールへ格納しておく事前ロード処理を実装し、戦闘中のリアルタイム生成による処理落ち（ヒッチ）を防止しています。

### 3. ネットワークマルチプレイへの対応と正当な権限管理

オンラインプレイを前提としたネットワークマルチ処理をC++で実装しています。

- **サーバー権限（Authority）の徹底**: ダメージ計算、ステータスの追加など、ゲームの進行に重要なロジックは必ずサーバー側（`HasAuthority()`）で処理し、チートや同期ズレを防ぐ適切な設計をしています。
- **レプリケーションと同期**: `GetLifetimeReplicatedProps` を用いて同期が必要な変数（HPやスタミナ、装備中の武器情報など）のみを正しくレプリケーションし、`OnRep`関数を利用してクライアント側での表示更新（UIやアニメーション）をフックさせる処理を実装しています。

### 4. 拡張性を意識したコンポーネント指向なアーキテクチャ

機能ごとに`ActorComponent`へ分割し、オブジェクト間の疎結合な設計を実現しています。

- **AC_SpellComponent**: 呪文の付与と解除を行い、呪文のコスト制限や呪文のスロットにはいるかどうかを厳格にチェックするマルチプレイ対応コンポーネント。
- **AC_EquipmentComponent**: 装備品とインベントリを管理し、GASと連携するマルチプレイ対応コンポーネント。アイテム装備に必要な機能を実装しています。
- **AC_StatComponent**: 装備品に付与されているGameplayEffectをステータスに付与するマルチプレイ対応コンポーネント。
- **AC_WeaponComponent**: 武器の生成・収納・着脱を一括管理するマルチプレイ対応コンポーネント。

## 📁 評価していただきたい主要なクラス

コードレビューの際の参考として、アーキテクチャの根幹となる主要なクラスをピックアップしました。

| クラス / コンポーネント | 概要 |
|---|---|
| [`AC_SpellComponent.cpp`](Source/GAS/SpellSystem/SpellComponent/AC_SpellComponent.cpp) | 習得した魔法の装備・スロット管理コンポーネント。GAS（AttributeSet）から取得した「最大記憶容量」に対するリアルタイムなコスト制限計算や、一意装備チェック、ServerRPCによるネットワーク同期を実装しています。 |
| [`AC_EquipmentComponent.cpp`](Source/GAS/EquipmentSystem/EquipmentComponent/AC_EquipmentComponent.cpp) | グリッド形式のインベントリと装備スロットを統合管理するマルチプレイ対応コンポーネント。装備変更に伴うGAS（GameplayEffect）の動的付与・解除や、他プレイヤー（死体等）とのアイテム転送ロジックをサーバー権限で安全に処理します。 |
| [`AC_StatComponent.cpp`](Source/GAS/StatSystem/AC_StatComponent.cpp) | ステータス割り振りを制御するコンポーネント。SetByCallerを用いてGameplayEffect経由で動的にステータス数値を上昇させる仕組みや、サーバー検証による不正防止を実装しています。 |
|[`AC_WeaponComponent.cpp`](Source/GAS/Weapon/WeaponComponent/AC_WeaponComponent.cpp) | 武器の生成・収納・着脱を一括管理するマルチプレイ対応コンポーネント。サーバー権限での装備変更と`EquippedWeapon`のレプリケーション（`OnRep`）により、見た目のアタッチ、`AnimInstanceClass`の変更、`CharacterMovementComponent`（移動速度や回転制御）のパラメータ設定を各クライアントへ正しく同期します。 |
| [`ActorPoolSubsystem.cpp`](Source/GAS/WorldSubsystem/ActorPoolSubsystem.cpp) | `UWorldSubsystem` を継承した汎用オブジェクトプール。`IPoolableActor` インターフェースを用いた状態初期化や事前生成（`Prewarm`）により、弾丸やエフェクト生成時のヒッチ（処理落ち）を防止します。 |
| [`DamageCalculationModifier.cpp`](Source/GAS/GameplayAbilitySystem/Effects/DamageCalculationModifier.cpp) | GASのカスタム計算クラス（MMC）。GameplayEffectのアセットタグ（`Data.DamageType`）から物理/魔法判定を行い、ソースの`PhysicalPower`/`MagicalPower`およびターゲットの`Armor`を動的にキャプチャして最終ダメージを算出します。さらに、ターゲットの`Status.Buff.Shield`タグを検知してダメージを0にする等、タグ連携による状態判定を組み込んでいます。 | |
| [`BTD_StateGameplayTag.cpp`](Source/GAS/AI/Decorator/BTD_StateGameplayTag.cpp) | ビヘイビアツリーのカスタムデコレーター。Tickを使わずに、ASCのGameplayTagの変更をDelegateで監視するイベント駆動の仕組みを実装しています。 |


## 開発を通じて得た知見・今後の展望

### 💡 開発を通じて得た知見
- **GAS（Gameplay Ability System）の設計思想の理解**
  Ability、GameplayEffect、AttributeSet を適切に責務分離することで、スキルや魔法を追加する際の拡張性・再利用性が大幅に向上することを実感しました。
- **サーバー権限とネットワーク同期の基礎**
  チート防止のためのサーバー権限（Authority）管理や、`OnRep`・RPCを用いた適切な状態同期の重要性を学びました。

### 🚀 今後の展望
- **ネットワーク技術のより深い理解と最適化の追求**
 - **クライアント予測（Prediction）とラグ補償の高度化**  
  アクション性の高い魔法・攻撃処理において、クライアント側の即時応答性とサーバー側の正当性を高いレベルで両立させる予測メカニズムを深掘りします。
- **通信帯域（Bandwidth）の最適化**  
  `NetCondition` の精査や不要なRPCの削減を通じ、多数のアクターが存在する状況下での負荷軽減を追求します。
- **Dedicated Server環境での実効性の検証**  
  人工的なネットワーク遅延（Ping）環境下で、同期ズレや表示破綻を防ぐ堅牢な実装を検証します。
 
## ライセンス

本リポジトリはポートフォリオ（C++ソースコード評価）目的で公開しています。
<!-- 再利用や二次利用の可否を明記する場合はこちらにライセンス（MIT等）を記載してください。 -->
