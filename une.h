/*
  une.h
  うね1匹分のクラス

  Copyright (c) 2025 Kaz  (https://akibabara.com/blog/)
  Released under the MIT license.
  see https://opensource.org/licenses/MIT
*/
#pragma once
#include <M5Unified.h>

// デバッグに便利なマクロ定義 --------
#define sp(x) Serial.println(x)
#define spn(x) Serial.print(x)
#define spp(k,v) Serial.println(String(k)+"="+String(v))
#define spf(fmt, ...) Serial.printf(fmt, __VA_ARGS__)
#define array_length(x) (sizeof(x) / sizeof(x[0]))


// うね1匹分のクラス
class Une {
public:
  // メンバ変数
  LovyanGFX* _dst;  // 出力先キャンバス
  M5Canvas* _src;  // 読み込み元のうねのキャンバス
  bool* _lcdBusyPtr = nullptr;    // LCDをクラス間で排他処理するためのフラグ　へのポインター

  // うねの設定
  int num = 10;   // 胴体の数
  static const int max_num = 20;  // 胴体の最大数
  float speed = 0.2;    // うねの移動速度
  uint16_t _transparent = 0x0000;  // 透明色

  // うねの状態
  float target_x=0, target_y=0;  // ターゲットの位置
  float ux[max_num], uy[max_num];  // 胴体の位置
  bool finish = true;  // ターゲットに到着したか

  // コンストラクタ
  Une(LovyanGFX* dst, M5Canvas* src, uint16_t transparent, int unenum, bool* lcdBusyPointer);
  ~Une() = default;

  // うねのターゲットを設定（xy:-1.0～1.0の範囲で設定）
  void target(float x, float y);

  // うねの移動（計算のみ）
  void update();

  // うねをキャンバスに描画
  void draw(int center_x, int center_y);
 
};
