/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once // インクルードガード。このヘッダーファイルが複数回インクルードされることを防ぎます。

// HAL (Hardware Abstraction Layer) の基本インターフェースを定義するヘッダーファイルをインクルードします。
// HalEsp32クラスはこのHalBaseクラスを継承します。
#include <hal/hal.h>

// INA226 電流・電力モニターICを制御するためのライブラリをインクルードします。
#include <ina226.hpp>

// LVGL (Light and Versatile Graphics Library) のコアヘッダーファイルをインクルードします。
// GUI関連の機能を提供します。
#include <lvgl.h>

// RX8130 リアルタイムクロック(RTC)ICを制御するためのライブラリをインクルードします。
#include "utils/rx8130/rx8130.h"

// HalEsp32クラスは、hal::HalBaseクラスをパブリック継承します。
// これにより、ESP32プラットフォーム固有のハードウェア操作を抽象化し、
// アプリケーションフレームワークに対して統一されたインターフェースを提供します。
class HalEsp32 : public hal::HalBase {
public:
    // このHAL実装のタイプを返す純粋仮想関数のオーバーライドです。
    // "Tab5" という文字列を返し、このHALがM5Stack Tab5デバイス向けであることを示します。
    std::string type() override
    {
        return "Tab5";
    }

    // ハードウェアの初期化処理を行う純粋仮想関数のオーバーライドです。
    // この関数内で、ディスプレイ、センサー、ペリフェラルなどの初期設定が行われます。
    void init() override;

    // 指定されたミリ秒数だけ処理を遅延させる純粋仮想関数のオーバーライドです。
    void delay(uint32_t ms) override;

    // システム起動からの経過時間をミリ秒単位で返す純粋仮想関数のオーバーライドです。
    uint32_t millis() override;

    // CPUの温度を取得する純粋仮想関数のオーバーライドです。
    int getCpuTemp() override;

    // INA226 電流・電力モニターICのインスタンスです。
    // これを通じて、バッテリー電圧や消費電流などを監視できます。
    INA226 ina226;

    // RX8130 RTC ICのインスタンスです。
    // これを通じて、現在時刻の取得や設定、アラーム機能などを利用できます。
    RX8130_Class rx8130;

    // LVGLが使用するディスプレイデバイスへのポインタです。
    // ディスプレイの描画処理に関連します。
    lv_disp_t* lvDisp      = nullptr;

    // LVGLが使用する入力デバイス (キーボードやタッチパッドなど) へのポインタです。
    // この例では主にタッチパッドからの入力を扱います。
    lv_indev_t* lvKeyboard = nullptr;

    // ディスプレイの輝度を設定する純粋仮想関数のオーバーライドです。
    // brightness は 0 から 100 の範囲で指定します。
    void setDisplayBrightness(uint8_t brightness) override;

    // 現在のディスプレイ輝度を取得する純粋仮想関数のオーバーライドです。
    uint8_t getDisplayBrightness() override;

    // LVGLの描画処理中に排他制御を行うためのロック関数のオーバーライドです。
    // マルチタスク環境でLVGLのデータ構造を保護します。
    void lvglLock() override;

    // LVGLの排他制御を解除するためのアンロック関数のオーバーライドです。
    void lvglUnlock() override;

    // 電源モニター (INA226) のデータを更新する純粋仮想関数のオーバーライドです。
    void updatePowerMonitorData() override;

    // IMU (慣性計測ユニット) のデータを更新する純粋仮想関数のオーバーライドです。
    void updateImuData() override;

    // IMUの割り込みフラグをクリアする純粋仮想関数のオーバーライドです。
    void clearImuIrq() override;

    // RTCの割り込みフラグをクリアする純粋仮想関数のオーバーライドです。
    void clearRtcIrq() override;

    // RTCの時刻を設定する純粋仮想関数のオーバーライドです。
    // tm構造体で指定された時刻を設定します。
    void setRtcTime(tm time) override;

    // 充電ICのQuick Charge (QC)機能を有効/無効にする純粋仮想関数のオーバーライドです。
    void setChargeQcEnable(bool enable) override;

    // 現在の充電ICのQC機能の状態を取得する純粋仮想関数のオーバーライドです。
    bool getChargeQcEnable() override;

    // 充電機能を有効/無効にする純粋仮想関数のオーバーライドです。
    void setChargeEnable(bool enable) override;

    // 現在の充電機能の状態を取得する純粋仮想関数のオーバーライドです。
    bool getChargeEnable() override;

    // USBポートからの5V出力を有効/無効にする純粋仮想関数のオーバーライドです。
    void setUsb5vEnable(bool enable) override;

    // 現在のUSBポートからの5V出力の状態を取得する純粋仮想関数のオーバーライドです。
    bool getUsb5vEnable() override;

    // 外部5V出力を有効/無効にする純粋仮想関数のオーバーライドです。
    void setExt5vEnable(bool enable) override;

    // 現在の外部5V出力の状態を取得する純粋仮想関数のオーバーライドです。
    bool getExt5vEnable() override;

    // デバイスの電源をオフにする純粋仮想関数のオーバーライドです。
    void powerOff() override;

    // タッチ入力によるウェイクアップを伴うスリープモードに移行する純粋仮想関数のオーバーライドです。
    void sleepAndTouchWakeup() override;

    // IMU (加速度センサーなど) の揺れ検出によるウェイクアップを伴うスリープモードに移行する純粋仮想関数のオーバーライドです。
    void sleepAndShakeWakeup() override;

    // RTCアラームによるウェイクアップを伴うスリープモードに移行する純粋仮想関数のオーバーライドです。
    void sleepAndRtcWakeup() override;

    // カメラキャプチャを開始し、指定されたLVGLイメージキャンバスに映像を表示する純粋仮想関数のオーバーライドです。
    void startCameraCapture(lv_obj_t* imgCanvas) override;

    // カメラキャプチャを停止する純粋仮想関数のオーバーライドです。
    void stopCameraCapture() override;

    // カメラが現在キャプチャ中かどうかを返す純粋仮想関数のオーバーライドです。
    bool isCameraCapturing() override;

    // スピーカーの音量を設定する純粋仮想関数のオーバーライドです。
    // volume は 0 から 100 の範囲で指定します。
    void setSpeakerVolume(uint8_t volume) override;

    // 現在のスピーカー音量を取得する純粋仮想関数のオーバーライドです。
    uint8_t getSpeakerVolume() override;

    // 指定された期間、マイクから音声を録音し、dataベクターに格納する純粋仮想関数のオーバーライドです。
    // gainで録音ゲインを調整できます。
    void audioRecord(std::vector<int16_t>& data, uint16_t durationMs, float gain = 80.0f) override;

    // 指定された音声データをスピーカーから再生する純粋仮想関数のオーバーライドです。
    // asyncがtrueの場合、非同期で再生します。
    void audioPlay(std::vector<int16_t>& data, bool async = true) override;

    // デュアルマイクの録音テストを開始する純粋仮想関数のオーバーライドです。
    void startDualMicRecordTest() override;

    // デュアルマイク録音テストの状態を取得する純粋仮想関数のオーバーライドです。
    MicTestState_t getDualMicRecordTestState() override;

    // ヘッドフォンマイクの録音テストを開始する純粋仮想関数のオーバーライドです。
    void startHeadphoneMicRecordTest() override;

    // ヘッドフォンマイク録音テストの状態を取得する純粋仮想関数のオーバーライドです。
    MicTestState_t getHeadphoneMicRecordTestState() override;

    // 音楽再生テストを開始する純粋仮想関数のオーバーライドです。
    void startPlayMusicTest() override;

    // 音楽再生テストの状態を取得する純粋仮想関数のオーバーライドです。
    MusicPlayState_t getMusicPlayTestState() override;

    // 音楽再生テストを停止する純粋仮想関数のオーバーライドです。
    void stopPlayMusicTest() override;

    // 起動時の効果音を再生する純粋仮想関数のオーバーライドです。
    void playStartupSfx() override;

    // シャットダウン時の効果音を再生する純粋仮想関数のオーバーライドです。
    void playShutdownSfx() override;

    // 外部アンテナを有効/無効にする純粋仮想関数のオーバーライドです。
    void setExtAntennaEnable(bool enable) override;

    // 現在の外部アンテナの状態を取得する純粋仮想関数のオーバーライドです。
    bool getExtAntennaEnable() override;

    // Wi-Fiアクセスポイントモードを開始する純粋仮想関数のオーバーライドです。
    void startWifiAp() override;

    // SDカードがマウントされているかどうかを返す純粋仮想関数のオーバーライドです。
    bool isSdCardMounted() override;

    // SDカードの指定されたディレクトリパス内のファイルとディレクトリのリストをスキャンして返す純粋仮想関数のオーバーライドです。
    std::vector<FileEntry_t> scanSdCard(const std::string& dirPath) override;

    // USB Type-Cポートの接続状態を検出する純粋仮想関数のオーバーライドです。
    bool usbCDetect() override;

    // USB Type-Aポートの接続状態を検出する純粋仮想関数のオーバーライドです。
    bool usbADetect() override;

    // ヘッドフォンジャックの接続状態を検出する純粋仮想関数のオーバーライドです。
    bool headPhoneDetect() override;

    // I2Cバスをスキャンし、接続されているデバイスのアドレスリストを返す純粋仮想関数のオーバーライドです。
    // isInternalがtrueの場合は内部I2Cバスを、falseの場合は外部I2Cバス (Port A) をスキャンします。
    std::vector<uint8_t> i2cScan(bool isInternal) override;

    // Port A のI2Cインターフェースを初期化する純粋仮想関数のオーバーライドです。
    void initPortAI2c() override;

    // Port A のI2Cインターフェースを終了処理する純粋仮想関数のオーバーライドです。
    void deinitPortAI2c() override;

    // 指定されたGPIOピンを出力モードとして初期化する純粋仮想関数のオーバーライドです。
    void gpioInitOutput(uint8_t pin) override;

    // 指定されたGPIOピンの出力レベルを設定する純粋仮想関数のオーバーライドです。
    void gpioSetLevel(uint8_t pin, bool level) override;

    // 指定されたGPIOピンをリセット (通常はLowレベルに設定) する純粋仮想関数のオーバーライドです。
    void gpioReset(uint8_t pin) override;

private:
    // GPIOピンの出力駆動能力を設定するプライベートヘルパー関数です。
    void set_gpio_output_capability();

    // HID (Human Interface Device) 関連の初期化を行うプライベートヘルパー関数です。
    // USBキーボードやマウスなどの接続を処理します。
    void hid_init();

    // RS485通信の初期化を行うプライベートヘルパー関数です。
    void rs485_init();

    // Wi-Fi関連の初期化を行うプライベートヘルパー関数です。
    bool wifi_init();

    // IMU (慣性計測ユニット) の初期化を行うプライベートヘルパー関数です。
    void imu_init();

    // システム時刻をRTCから読み出して更新するプライベートヘルパー関数です。
    void update_system_time();

    // 現在のLCDバックライト輝度を保持するメンバー変数です。(0-100)
    uint8_t _current_lcd_brightness = 100;

    // 充電ICのQuick Charge機能の有効/無効状態を保持するメンバー変数です。
    bool _charge_qc_enable          = false;

    // 充電機能の有効/無効状態を保持するメンバー変数です。
    bool _charge_enable             = true;

    // 外部5V出力の有効/無効状態を保持するメンバー変数です。
    bool _ext_5v_enable             = true;

    // USB-Aポートからの5V出力の有効/無効状態を保持するメンバー変数です。
    bool _usba_5v_enable            = true;

    // 外部アンテナの有効/無効状態を保持するメンバー変数です。
    bool _ext_antenna_enable        = false;

    // SDカードがマウントされているかどうかの状態を保持するメンバー変数です。
    bool _sd_card_mounted           = false;
};

