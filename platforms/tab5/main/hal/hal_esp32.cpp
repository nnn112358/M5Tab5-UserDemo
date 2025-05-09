/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

// HalEsp32クラスの宣言を含むヘッダーファイルをインクルードします。
#include "hal/hal_esp32.h"

// C言語スタイルのヘッダーファイルをインクルードする場合に extern "C" を使用します。
// ここではRX8130 RTCドライバのヘッダーをインクルードしています。
extern "C" {
#include "utils/rx8130/rx8130.h"
}

// Mooncakeライブラリのログ機能を使用するためのヘッダーファイルをインクルードします。
#include <mooncake_log.h>

// ESP-IDFのタイマー機能 (高精度タイマー) を使用するためのヘッダーファイルをインクルードします。
#include <esp_timer.h>

// FreeRTOSの基本機能とタスク管理機能を使用するためのヘッダーファイルをインクルードします。
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// M5Stack Tab5のBSP (Board Support Package) ライブラリをインクルードします。
// これには、ディスプレイ、タッチ、オーディオコーデックなどのハードウェア初期化・制御関数が含まれます。
#include <bsp/m5stack_tab5.h>

// LVGLのデモアプリケーションに関連するヘッダーファイルです。
// (このファイルでは直接使用されていませんが、プロジェクト全体で関連する可能性があります)
#include <lv_demos.h>

// BSP内部で定義されているLCDタッチハンドラへの外部参照です。
// これを通じてタッチ入力データを取得します。
extern esp_lcd_touch_handle_t _lcd_touch_handle;

// このモジュール用のログ出力に使用するタグ文字列を定義します。
static const std::string _tag = "hal";

// LVGLの入力デバイス (タッチパッド) の読み取りコールバック関数です。
// LVGLは定期的にこの関数を呼び出し、タッチの状態と座標を取得します。
static void lvgl_read_cb(lv_indev_t* indev, lv_indev_data_t* data)
{
    // タッチハンドラが無効な場合は、リリース状態としてLVGLに通知します。
    if (_lcd_touch_handle == NULL) {
        data->state = LV_INDEV_STATE_REL;
        return;
    }

    // タッチ座標、強度、タッチポイント数を格納する変数を宣言します。
    uint16_t touch_x[1];
    uint16_t touch_y[1];
    uint16_t touch_strength[1];
    uint8_t touch_cnt = 0;

    // LCDタッチコントローラから最新のタッチデータを読み取ります。
    esp_lcd_touch_read_data(_lcd_touch_handle);

    // 読み取ったデータから、タッチ座標、強度、タッチポイント数を取得します。
    // touchpad_pressed はタッチされているかどうかを示します。
    bool touchpad_pressed =
        esp_lcd_touch_get_coordinates(_lcd_touch_handle, touch_x, touch_y, touch_strength, &touch_cnt, 1);
    // mclog::tagInfo(_tag, "touchpad pressed: {}", touchpad_pressed); // デバッグ用のログ出力 (コメントアウトされています)

    // タッチされていない場合は、LVGLにリリース状態 (LV_INDEV_STATE_REL) を通知します。
    if (!touchpad_pressed) {
        data->state = LV_INDEV_STATE_REL;
    } else {
        // タッチされている場合は、LVGLにプレス状態 (LV_INDEV_STATE_PR) を通知し、
        // 取得したタッチ座標 (touch_x[0], touch_y[0]) をLVGLに渡します。
        data->state   = LV_INDEV_STATE_PR;
        data->point.x = touch_x[0];
        data->point.y = touch_y[0];
    }
}

// HalEsp32クラスの初期化関数です。各種ハードウェアの初期設定を行います。
void HalEsp32::init()
{
    mclog::tagInfo(_tag, "init"); // 初期化開始のログ出力

    mclog::tagInfo(_tag, "camera init"); // カメラ初期化開始のログ出力
    bsp_cam_osc_init(); // カメラモジュール用のオシレータを初期化します。

    mclog::tagInfo(_tag, "i2c init"); // I2C初期化開始のログ出力
    bsp_i2c_init(); // 内部I2Cバスを初期化します。

    mclog::tagInfo(_tag, "io expander init"); // IOエキスパンダ初期化開始のログ出力
    i2c_master_bus_handle_t i2c_bus_handle = bsp_i2c_get_handle(); // 初期化済みのI2Cバスハンドルを取得します。
    bsp_io_expander_pi4ioe_init(i2c_bus_handle); // PI4IOE5V9539 IOエキスパンダを初期化します。

    // 充電ICのQuick Charge機能を有効にし、少し遅延を入れます。
    setChargeQcEnable(true);
    delay(50);
    // 充電機能を有効にします。(コメントアウトされている行は、開発中に充電を無効にするためのものかもしれません)
    setChargeEnable(true);
    // setChargeEnable(false);

    mclog::tagInfo(_tag, "i2c scan"); // I2Cバススキャン開始のログ出力
    bsp_i2c_scan(); // 接続されているI2Cデバイスをスキャンし、ログに出力します。

    mclog::tagInfo(_tag, "codec init"); // オーディオコーデック初期化開始のログ出力
    delay(200); // コーデックの安定化のために少し遅延を入れます。
    bsp_codec_init(); // オーディオコーデック (ES8311) を初期化します。

    mclog::tagInfo(_tag, "imu init"); // IMU初期化開始のログ出力
    imu_init(); // IMU (慣性計測ユニット、例: BMI270) を初期化します。

    mclog::tagInfo(_tag, "ina226 init"); // INA226電流センサー初期化開始のログ出力
    // INA226をI2Cバスハンドルとアドレス(0x41)を指定して初期化します。
    ina226.begin(i2c_bus_handle, 0x41);
    // INA226の動作モードを設定します (平均化回数、バス電圧変換時間、シャント電圧変換時間、動作モード)。
    ina226.configure(INA226_AVERAGES_16, INA226_BUS_CONV_TIME_1100US, INA226_SHUNT_CONV_TIME_1100US,
                     INA226_MODE_SHUNT_BUS_CONT);
    // INA226をキャリブレーションします (シャント抵抗値[Ohm], 最大期待電流[A])。
    ina226.calibrate(0.005, 8.192);
    mclog::tagInfo(_tag, "bus voltage: {}", ina226.readBusVoltage()); // バス電圧を読み取りログに出力します。

    mclog::tagInfo(_tag, "rx8130 init"); // RX8130 RTC初期化開始のログ出力
    // RX8130をI2Cバスハンドルとアドレス(0x32)を指定して初期化します。
    rx8130.begin(i2c_bus_handle, 0x32);
    rx8130.initBat(); // RTCのバックアップバッテリー関連の初期化を行います。
    clearRtcIrq();    // RTCの割り込みフラグをクリアします。
    update_system_time(); // システム時刻をRTCから読み出して更新します。

    mclog::tagInfo(_tag, "display init"); // ディスプレイ初期化開始のログ出力
    bsp_reset_tp(); // タッチパネルをリセットします。

    // ディスプレイ設定構造体を準備します。
    bsp_display_cfg_t cfg = {.lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(), // LVGLポートのデフォルト設定を使用します。
                             .buffer_size   = BSP_LCD_H_RES * BSP_LCD_V_RES, // フレームバッファサイズ (フルスクリーン)。
                             .double_buffer = true, // ダブルバッファリングを有効にします。
                             .flags         = {
#if CONFIG_BSP_LCD_COLOR_FORMAT_RGB888 // RGB888カラーフォーマットの場合
                                 .buff_dma = false, // DMA転送を無効 (RGB888ではCPUコピーが速い場合がある)
#else // RGB565などの場合
                                 .buff_dma = true,  // DMA転送を有効
#endif
                                 .buff_spiram = true, // フレームバッファをSPIRAMに配置します。
                                 .sw_rotate   = true,   // ソフトウェアによる画面回転を有効にします。
                             }};
    // 上記設定でディスプレイを開始し、LVGLディスプレイハンドルを取得します。
    lvDisp = bsp_display_start_with_config(&cfg);
    // ディスプレイの回転を90度に設定します (縦向き)。
    lv_display_set_rotation(lvDisp, LV_DISPLAY_ROTATION_90);
    // ディスプレイのバックライトをオンにします。
    bsp_display_backlight_on();

    // LVGL用のタッチパッド入力デバイスを作成します。
    mclog::tagInfo(_tag, "create lvgl touchpad indev");
    lvTouchpad = lv_indev_create(); // LVGL入力デバイスを作成
    lv_indev_set_type(lvTouchpad, LV_INDEV_TYPE_POINTER); // タイプをポインティングデバイスに設定
    lv_indev_set_read_cb(lvTouchpad, lvgl_read_cb); // 読み取りコールバック関数を登録
    lv_indev_set_display(lvTouchpad, lvDisp); // 関連付けるディスプレイを設定

    mclog::tagInfo(_tag, "usb host init"); // USBホスト初期化開始のログ出力
    // USBホスト機能を初期化します。電源モードと自動ファームウェアロードを設定します。
    bsp_usb_host_start(BSP_USB_HOST_POWER_MODE_USB_DEV, true);

    mclog::tagInfo(_tag, "hid init"); // HID初期化開始のログ出力
    hid_init(); // USB HID (キーボード、マウスなど) の処理を初期化します。

    mclog::tagInfo(_tag, "rs485 init"); // RS485初期化開始のログ出力
    rs485_init(); // RS485通信インターフェースを初期化します。

    mclog::tagInfo(_tag, "set gpio output capability"); // GPIO出力能力設定開始のログ出力
    set_gpio_output_capability(); // 特定GPIOピンの駆動能力を設定します。

    bsp_display_unlock(); // ディスプレイのロックを解除します (LVGLの準備ができたことを示す)。
}

// 駆動能力を調整するGPIOピンのリストです。
// これらのピンは、特定のペリフェラルや外部コンポーネントを駆動するために、
// 標準よりも低い駆動能力(GPIO_DRIVE_CAP_0)に設定されることがあります。
// これにより、信号品質の改善や消費電力の削減が期待できる場合があります。
static const gpio_num_t _driver_gpios[] = {
    // 外部I2C (Port A)
    GPIO_NUM_0,
    GPIO_NUM_1,
    // ESP-Hosted (ESP32C6 Wi-Fiコプロセッサとの通信用)
    GPIO_NUM_8,
    GPIO_NUM_9,
    GPIO_NUM_10,
    GPIO_NUM_11,
    GPIO_NUM_12,
    GPIO_NUM_13,
    GPIO_NUM_15,
    // ディスプレイインターフェース
    GPIO_NUM_22,
    GPIO_NUM_23,
    // オーディオインターフェース
    GPIO_NUM_26,
    GPIO_NUM_27,
    GPIO_NUM_28,
    GPIO_NUM_29,
    GPIO_NUM_30,
    // システムI2C (内部)
    GPIO_NUM_31,
    GPIO_NUM_32,
    // uSDカードインターフェース
    GPIO_NUM_39,
    GPIO_NUM_40,
    GPIO_NUM_41,
    GPIO_NUM_42,
    GPIO_NUM_43,
    GPIO_NUM_44,
};

// 指定されたGPIOピンの出力駆動能力を設定するヘルパー関数です。
void HalEsp32::set_gpio_output_capability()
{
    // gpio_set_drive_capability((gpio_num_t)48, GPIO_DRIVE_CAP_0); // 特定ピンの例 (コメントアウト)
    // _driver_gpios配列内の各GPIOピンに対してループ処理を行います。
    for (int i = 0; i < sizeof(_driver_gpios) / sizeof(_driver_gpios[0]); i++) {
        gpio_num_t gpio = _driver_gpios[i];
        // GPIOの駆動能力を GPIO_DRIVE_CAP_0 (最も低い能力) に設定します。
        esp_err_t ret   = gpio_set_drive_capability(gpio, GPIO_DRIVE_CAP_0);
        if (ret == ESP_OK) {
            printf("GPIO %d drive capability set to GPIO_DRIVE_CAP_0\n", gpio);
        } else {
            printf("Failed to set GPIO %d drive capability: %s\n", gpio, esp_err_to_name(ret));
        }
    }
}

/* -------------------------------------------------------------------------- */
/*                                   System                                   */
/* -------------------------------------------------------------------------- */
#include <driver/temperature_sensor.h> // ESP32内蔵温度センサー用ドライバ
static temperature_sensor_handle_t _temp_sensor = nullptr; // 温度センサーハンドラ

// 指定されたミリ秒数だけ現在のタスクを遅延させます。
void HalEsp32::delay(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms)); // FreeRTOSのvTaskDelayを使用。pdMS_TO_TICKSはミリ秒をTick数に変換するマクロ。
}

// システム起動からの経過時間をミリ秒単位で返します。
uint32_t HalEsp32::millis()
{
    return esp_timer_get_time() / 1000; // ESP-IDFの高精度タイマーの値 (マイクロ秒単位) を1000で割ってミリ秒に変換。
}

// CPUの内部温度を摂氏で返します。
int HalEsp32::getCpuTemp()
{
    // 温度センサーがまだ初期化されていない場合、初期化を行います。
    if (_temp_sensor == nullptr) {
        temperature_sensor_config_t temp_sensor_config = {
            .range_min = 20, // 測定範囲の最小値 (この値はESP32のバージョンによって異なる場合がある)
            .range_max = 100, // 測定範囲の最大値
        };
        temperature_sensor_install(&temp_sensor_config, &_temp_sensor); // 温度センサーをインストール
        temperature_sensor_enable(_temp_sensor); // 温度センサーを有効化
    }

    float temp = 0;
    temperature_sensor_get_celsius(_temp_sensor, &temp); // 温度を摂氏で取得

    return temp; // 整数にキャストして返す (必要に応じて丸め処理を検討)
}

/* -------------------------------------------------------------------------- */
/*                                   Display                                  */
/* -------------------------------------------------------------------------- */
// ディスプレイのバックライト輝度を設定します (0-100%)。
void HalEsp32::setDisplayBrightness(uint8_t brightness)
{
    // 明るさの値を0から100の範囲にクランプします。
    _current_lcd_brightness = std::clamp((int)brightness, 0, 100);
    mclog::tagInfo("hal", "set display brightness: {}%", _current_lcd_brightness);
    bsp_display_brightness_set(_current_lcd_brightness); // BSP関数を呼び出して実際の輝度を設定
}

// 現在のディスプレイバックライト輝度を取得します。
uint8_t HalEsp32::getDisplayBrightness()
{
    return _current_lcd_brightness;
}

// LVGL操作のためのロックを取得します (ミューテックスなどによる排他制御)。
// マルチスレッド環境でLVGLの内部状態を保護するために使用します。
void HalEsp32::lvglLock()
{
    lvgl_port_lock(0); // LVGLポート提供のロック関数を呼び出し
}

// LVGL操作のためのロックを解放します。
void HalEsp32::lvglUnlock()
{
    lvgl_port_unlock(); // LVGLポート提供のアンロック関数を呼び出し
}

/* -------------------------------------------------------------------------- */
/*                                     RTC                                    */
/* -------------------------------------------------------------------------- */
// RTC (RX8130) の割り込みフラグをクリアします。
void HalEsp32::clearRtcIrq()
{
    mclog::tagInfo(_tag, "clear rtc irq");
    rx8130.clearIrqFlags(); // RX8130ドライバの関数で割り込みフラグをクリア
    rx8130.disableIrq();    // RX8130の割り込みを無効化 (必要に応じて)
}

// RTCに時刻を設定します。
void HalEsp32::setRtcTime(tm time)
{
    // 設定する時刻をログに出力します。
    mclog::tagInfo(_tag, "set rtc time to {}/{}/{} {:02d}:{:02d}:{:02d}", time.tm_year + 1900, time.tm_mon + 1,
                   time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
    rx8130.setTime(&time); // RX8130ドライバの関数で時刻を設定
    delay(50); // 設定が反映されるのを待つために少し遅延

    update_system_time(); // システム時刻もRTCに合わせて更新
}

// システム時刻をRTCから読み出して更新します。
void HalEsp32::update_system_time()
{
    mclog::tagInfo(_tag, "update system time");
    struct tm time; // 時刻情報を格納するtm構造体
    rx8130.getTime(&time); // RX8130から現在の時刻を取得
    // 取得したRTC時刻をログに出力します。
    mclog::tagInfo(_tag, "sync to rtc time: {}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}", time.tm_year + 1900,
                   time.tm_mon + 1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
    struct timeval now; // システム時刻設定用のtimeval構造体
    now.tv_sec  = mktime(&time); // tm構造体をUnixタイムスタンプ (秒) に変換
    now.tv_usec = 0; // マイクロ秒は0に設定
    settimeofday(&now, NULL); // システム時刻を設定 (Linux標準関数、ESP-IDFでも利用可能)
}

/* -------------------------------------------------------------------------- */
/*                                   SD Card                                  */
/* -------------------------------------------------------------------------- */
#include <dirent.h> // ディレクトリ操作関数 (opendir, readdir, closedir) 用
#include <sys/types.h> // DT_DIRなどの型定義用

// SDカードがマウントされているかどうかを返します。
// この実装では常にtrueを返していますが、実際にはbsp_sdcard_is_mounted()のような関数で状態を確認すべきです。
// M5Tab5のBSPでは、bsp_sdcard_init()の成否で判断することが多いです。
bool HalEsp32::isSdCardMounted()
{
    // return _sd_card_mounted; // 本来はこちらのように状態変数を見るべき
    return true; // 現状の実装は常にtrue
}

// SDカードの指定されたディレクトリパス内のファイルとディレクトリのリストをスキャンして返します。
std::vector<hal::HalBase::FileEntry_t> HalEsp32::scanSdCard(const std::string& dirPath)
{
    std::vector<hal::HalBase::FileEntry_t> file_entries; // 返却用のファイルエントリリスト

    mclog::tagInfo(_tag, "init sd card");
    // SDカードをマウントします。マウントポイントは "/sd"、CSピンは25 (これはハードウェアに依存)。
    // M5Tab5のデフォルトのCSピンとは異なる可能性があるため、BSPの定義を確認することが重要です。
    // この関数は呼び出しごとに初期化とデアロケーションを行うため、効率は良くありません。
    // アプリケーション側で一度だけ初期化し、マウント状態を保持する方が望ましいです。
    if (bsp_sdcard_init("/sd", 25) != ESP_OK) { // BSPのSDカード初期化関数
        mclog::error("failed to mount sd card");
        return file_entries; // マウント失敗時は空のリストを返す
    }
    _sd_card_mounted = true; // 状態変数を更新 (この関数内でのみ有効)

    // スキャン対象のフルパスを作成します (例: "/sd/my_directory")。
    std::string target_path = "/sd/" + dirPath;

    DIR* dir = opendir(target_path.c_str()); // ディレクトリを開く
    if (dir == nullptr) {
        mclog::error("failed to open directory: {}", target_path);
        bsp_sdcard_deinit("/sd"); // エラー時はSDカードをデアロケート
        _sd_card_mounted = false;
        return file_entries; // ディレクトリを開けなければ空のリストを返す
    }

    struct dirent* entry; // ディレクトリエントリ構造体へのポインタ
    // readdir() でディレクトリ内のエントリを順に読み取ります。
    while ((entry = readdir(dir)) != nullptr) {
        // カレントディレクトリ (".") と親ディレクトリ ("..") はスキップします。
        if (std::string(entry->d_name) == "." || std::string(entry->d_name) == "..") {
            continue;
        }

        hal::HalBase::FileEntry_t file_entry; // HALで定義されたファイルエントリ構造体
        file_entry.name  = entry->d_name; // ファイル名またはディレクトリ名
        file_entry.isDir = (entry->d_type == DT_DIR); // エントリがディレクトリかどうかを判定
        file_entries.push_back(file_entry); // リストに追加
    }

    closedir(dir); // 開いたディレクトリを閉じる

    mclog::tagInfo(_tag, "deinit sd card");
    bsp_sdcard_deinit("/sd"); // SDカードをデアロケート
    _sd_card_mounted = false;

    return file_entries; // スキャン結果のリストを返す
}

/* -------------------------------------------------------------------------- */
/*                                  Interface                                 */
/* -------------------------------------------------------------------------- */
// USB Type-Cポートの接続状態を検出します。
bool HalEsp32::usbCDetect()
{
    return bsp_usb_c_detect(); // BSPのUSB Type-C検出関数を呼び出し
    // return false; // テスト用の固定値 (コメントアウト)
}

// ヘッドフォンジャックの接続状態を検出します。
bool HalEsp32::headPhoneDetect()
{
    return bsp_headphone_detect(); // BSPのヘッドフォン検出関数を呼び出し
}

// I2Cバスをスキャンし、応答があったデバイスのアドレスリストを返します。
// isInternalがtrueなら内部I2Cバス、falseなら外部I2Cバス (Port A) をスキャンします。
std::vector<uint8_t> HalEsp32::i2cScan(bool isInternal)
{
    i2c_master_bus_handle_t i2c_bus_handle; // I2Cバスハンドル
    std::vector<uint8_t> addrs; // 発見したデバイスアドレスを格納するベクター

    if (isInternal) {
        i2c_bus_handle = bsp_i2c_get_handle(); // 内部I2Cバスのハンドルを取得
    } else {
        i2c_bus_handle = bsp_ext_i2c_get_handle(); // 外部I2Cバスのハンドルを取得
    }

    esp_err_t ret;
    uint8_t address;

    // I2Cアドレス空間 (0x00-0x7F) をスキャンします。
    // 一般的に0x00-0x07および0x78-0x7Fは予約済みなので、ここでは0x10からスキャンしています。
    // (ただし、一般的なスキャンは0x08から0x77まで行うことが多いです)
    for (int i = 16; i < 128; i += 16) { // 16アドレスごとにまとめて処理 (表示のためか？)
        for (int j = 0; j < 16; j++) {
            fflush(stdout); // 標準出力をフラッシュ (printfなどの出力を即座に行うため)
            address = i + j;
            if (address >= 0x78) continue; // 0x78以上のアドレスはスキップ
            // 指定したアドレスのデバイスにプローブ (短い通信試行) を行います。
            // タイムアウトは50msに設定。
            ret     = i2c_master_probe(i2c_bus_handle, address, 50);
            if (ret == ESP_OK) { // プローブ成功 (デバイスが応答した)
                addrs.push_back(address); // アドレスをリストに追加
            }
        }
    }
    return addrs; // 発見したアドレスのリストを返す
}

// Port A (外部I2C) を初期化します。
void HalEsp32::initPortAI2c()
{
    mclog::tagInfo(_tag, "init port a i2c");
    bsp_ext_i2c_init(); // BSPの外部I2C初期化関数を呼び出し
}

// Port A (外部I2C) を終了処理 (デアロケート) します。
void HalEsp32::deinitPortAI2c()
{
    mclog::tagInfo(_tag, "deinit port a i2c");
    bsp_ext_i2c_deinit(); // BSPの外部I2C終了関数を呼び出し
}

// 指定されたGPIOピンを出力モードとして初期化します。
void HalEsp32::gpioInitOutput(uint8_t pin)
{
    gpio_set_pull_mode((gpio_num_t)pin, GPIO_PULLUP_ONLY); // プルアップを設定 (ピンのデフォルト状態をHighにするため)
    gpio_set_direction((gpio_num_t)pin, GPIO_MODE_OUTPUT); // GPIO方向を出力に設定
}

// 指定されたGPIOピンの出力レベル (High/Low) を設定します。
void HalEsp32::gpioSetLevel(uint8_t pin, bool level)
{
    gpio_set_level((gpio_num_t)pin, level); // GPIOレベルを設定
}

// 指定されたGPIOピンをリセットします (通常はLowレベルに設定)。
void HalEsp32::gpioReset(uint8_t pin)
{
    gpio_set_level((gpio_num_t)pin, false); // GPIOレベルをLowに設定
}

// 以下はhal_esp32.hで宣言されているが、この.cppファイル内では実装が提供されていない関数群です。
// これらは、アプリケーションの要求に応じて、または将来の拡張のために宣言されている可能性があります。
// もしくは、他のファイル (例: hal_audio.cpp, hal_power.cpp など) で実装されているかもしれません。

// void HalEsp32::updatePowerMonitorData() override; // (hal_power.cpp で実装されている可能性が高い)
// void HalEsp32::updateImuData() override; // (hal_imu.cpp で実装されている可能性が高い)
// void HalEsp32::clearImuIrq() override; // (hal_imu.cpp で実装されている可能性が高い)

// void HalEsp32::setChargeQcEnable(bool enable) override; // (hal_power.cpp で実装されている可能性が高い)
// bool HalEsp32::getChargeQcEnable() override; // (hal_power.cpp で実装されている可能性が高い)
// void HalEsp32::setChargeEnable(bool enable) override; // (hal_power.cpp で実装されている可能性が高い)
// bool HalEsp32::getChargeEnable() override; // (hal_power.cpp で実装されている可能性が高い)
// void HalEsp32::setUsb5vEnable(bool enable) override; // (hal_power.cpp で実装されている可能性が高い)
// bool HalEsp32::getUsb5vEnable() override; // (hal_power.cpp で実装されている可能性が高い)
// void HalEsp32::setExt5vEnable(bool enable) override; // (hal_power.cpp で実装されている可能性が高い)
// bool HalEsp32::getExt5vEnable() override; // (hal_power.cpp で実装されている可能性が高い)
// void HalEsp32::powerOff() override; // (hal_power.cpp で実装されている可能性が高い)
// void HalEsp32::sleepAndTouchWakeup() override; // (hal_power.cpp で実装されている可能性が高い)
// void HalEsp32::sleepAndShakeWakeup() override; // (hal_power.cpp で実装されている可能性が高い)
// void HalEsp32::sleepAndRtcWakeup() override; // (hal_power.cpp で実装されている可能性が高い)

// void HalEsp32::startCameraCapture(lv_obj_t* imgCanvas) override; // (hal_camera.cpp で実装されている可能性が高い)
// void HalEsp32::stopCameraCapture() override; // (hal_camera.cpp で実装されている可能性が高い)
// bool HalEsp32::isCameraCapturing() override; // (hal_camera.cpp で実装されている可能性が高い)

// void HalEsp32::setSpeakerVolume(uint8_t volume) override; // (hal_audio.cpp で実装されている可能性が高い)
// uint8_t HalEsp32::getSpeakerVolume() override; // (hal_audio.cpp で実装されている可能性が高い)
// void HalEsp32::audioRecord(std::vector<int16_t>& data, uint16_t durationMs, float gain) override; // (hal_audio.cpp で実装されている可能性が高い)
// void HalEsp32::audioPlay(std::vector<int16_t>& data, bool async) override; // (hal_audio.cpp で実装されている可能性が高い)
// void HalEsp32::startDualMicRecordTest() override; // (hal_audio.cpp で実装されている可能性が高い)
// MicTestState_t HalEsp32::getDualMicRecordTestState() override; // (hal_audio.cpp で実装されている可能性が高い)
// void HalEsp32::startHeadphoneMicRecordTest() override; // (hal_audio.cpp で実装されている可能性が高い)
// MicTestState_t HalEsp32::getHeadphoneMicRecordTestState() override; // (hal_audio.cpp で実装されている可能性が高い)
// void HalEsp32::startPlayMusicTest() override; // (hal_audio.cpp で実装されている可能性が高い)
// MusicPlayState_t HalEsp32::getMusicPlayTestState() override; // (hal_audio.cpp で実装されている可能性が高い)
// void HalEsp32::stopPlayMusicTest() override; // (hal_audio.cpp で実装されている可能性が高い)
// void HalEsp32::playStartupSfx() override; // (hal_audio.cpp で実装されている可能性が高い)
// void HalEsp32::playShutdownSfx() override; // (hal_audio.cpp で実装されている可能性が高い)

// void HalEsp32::setExtAntennaEnable(bool enable) override; // (hal_wifi.cpp で実装されている可能性が高い)
// bool HalEsp32::getExtAntennaEnable() override; // (hal_wifi.cpp で実装されている可能性が高い)
// void HalEsp32::startWifiAp() override; // (hal_wifi.cpp で実装されている可能性が高い)

// bool HalEsp32::usbADetect() override; // (hal_usb.cpp で実装されている可能性が高い)

// プライベートヘルパー関数の実装
// void HalEsp32::hid_init() {} // (hal_usb.cpp や bsp で実装されている可能性が高い)
// void HalEsp32::rs485_init() {} // (hal_rs485.cpp で実装されている可能性が高い)
// bool HalEsp32::wifi_init() {} // (hal_wifi.cpp で実装されている可能性が高い)
// void HalEsp32::imu_init() {} // (hal_imu.cpp で実装されている可能性が高い)

// 注意: 上記のコメントアウトされた関数群は、このファイル (hal_esp32.cpp) には実装がありません。
// これらは通常、より具体的な機能を持つ別のHALコンポーネントファイル
// (例: platforms/tab5/main/hal/components/hal_power.cpp など) に実装されています。
// このhal_esp32.cppは、主にシステム全体の初期化フローや、
// 他のHALコンポーネントに分類されない基本的なハードウェア操作を担当しています。

