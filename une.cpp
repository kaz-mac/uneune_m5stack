/*
  une.cpp
  うね1匹分のクラス

  Copyright (c) 2025 Kaz  (https://akibabara.com/blog/)
  Released under the MIT license.
  see https://opensource.org/licenses/MIT
*/
#include "une.h"

// コンストラクタ
Une::Une(LovyanGFX* dst, M5Canvas* src, uint16_t transparent, int unenum, bool* lcdBusyPointer) {
  _dst = dst;
  _src = src;
  _transparent = transparent;
  num = unenum;
  _lcdBusyPtr = lcdBusyPointer;
  for (int i=0; i<max_num; i++) {
    ux[i] = 0;
    uy[i] = 0;
  }
  target_x = 0;
  target_y = 0;
  finish = true;
}

// うねのターゲットを設定
void Une::target(float x, float y) {
  target_x = x;
  target_y = y;
  finish = false;
}

// うねの移動（計算のみ）
void Une::update() {
  int head = num - 1;  // 頭のindex

  // ターゲットへのベクトルを計算
  float dx = target_x - ux[head];
  float dy = target_y - uy[head];
  float distance = sqrt(dx*dx + dy*dy);

  // 新ターゲットにすでに着いてたら終了
  if (distance <= 0.01) {
    finish = true;
    return;
  }
  
  // ターゲットに近付いたら減速していく
  float speed_effect = 1.0;
  if (distance <= speed*1.5) {
    speed_effect = 0.6 + (distance / (speed*1.5)) * (1.0 - 0.6);
  }
  
  // 正規化したベクトルにspeedを掛けて移動量を計算
  float move_x = (dx / distance) * speed * speed_effect;
  float move_y = (dy / distance) * speed * speed_effect;
  float move_distance = sqrt(move_x*move_x + move_y*move_y);  // 移動距離を計算
  
  // ターゲットを通過しないように制限
  if (move_distance >= distance) {
    // 移動距離がターゲットまでの距離以上の場合、ターゲット位置に直接移動
    ux[head] = target_x;
    uy[head] = target_y;
    finish = true;
  } else {
    // 通常の移動
    ux[head] += move_x;
    uy[head] += move_y;
  }

  // 胴体の座標を計算
  for (int i=num-2; i>=1; i--) {
    ux[i] = (ux[i+1] + ux[i-1]) / 2;
    uy[i] = (uy[i+1] + uy[i-1]) / 2;
  }
}

// うねをキャンバスに描画
void Une::draw(int center_x, int center_y) {
  int last_x = -1;
  int last_y = -1;
  int une_w = _src->width();
  int une_h = _src->height();
  int px_size = une_w;  // 1.0あたりの移動量 = 1うねの横幅

  for (int i=0; i<num; i++) {
    int x = center_x + ux[i] * px_size;
    int y = center_y + uy[i] * px_size;
    if (last_x != x || last_y != y) {   // 同じ座標の場合は描画しない
      // LCDの排他処理
      if (_lcdBusyPtr != nullptr) {
        while (*_lcdBusyPtr) delay(5);
        *_lcdBusyPtr = true;
      }
      // うねを描画
      _src->pushSprite(_dst, x-une_w/2, y-une_h/2, _transparent);
      // LCDの排他処理を解除
      if (_lcdBusyPtr != nullptr) *_lcdBusyPtr = false;
      last_x = x;
      last_y = y;
    }
  }
}

