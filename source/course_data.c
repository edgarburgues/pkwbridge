/* course_data.c - 27 Pokewalker course definitions
 * Each course lists its three encounter slots (NO1/NO2/NO3, first variant of each)
 * and ten dowsing items. Course names are stored as UTF-8.
 */
#include "course_data.h"

const course_def_t course_defs[COURSE_COUNT] = {
  /* ---- Course 1: さわやかのはら ---- */
  { .graphic_id=1, .course_id=1, .name_string="さわやかのはら",
    .enemies = {
        { .species=115, .level= 8, .held_item=  0, .moves={4, 43, 252, 0}, .form=0, .sex=1, .need_step= 3000, .odds= 50 }  /* ガルーラ (れんぞくパンチ/にらみつける/ねこだまし/なし) */,
        { .species= 29, .level= 5, .held_item=  0, .moves={45, 10, 0, 0}, .form=0, .sex=1, .need_step=  500, .odds= 75 }  /* ニドラン♀ (なきごえ/ひっかく/なし/なし) */,
        { .species= 16, .level= 5, .held_item=  0, .moves={33, 28, 0, 0}, .form=0, .sex=0, .need_step=    0, .odds=100 }  /* ポッポ (たいあたり/すなかけ/なし/なし) */
    },
    .items = {
        { .item_no= 28, .need_step= 2500, .odds= 20 }  /* げんきのかけら */,
        { .item_no= 27, .need_step= 2000, .odds= 20 }  /* なんでもなおし */,
        { .item_no= 19, .need_step= 1000, .odds= 30 }  /* やけどなおし */,
        { .item_no= 20, .need_step=  900, .odds= 30 }  /* こおりなおし */,
        { .item_no=150, .need_step=  800, .odds= 30 }  /* カゴのみ */,
        { .item_no= 21, .need_step=  700, .odds= 40 }  /* ねむけざまし */,
        { .item_no=149, .need_step=  600, .odds= 50 }  /* クラボのみ */,
        { .item_no= 22, .need_step=  500, .odds= 50 }  /* まひなおし */,
        { .item_no=155, .need_step=  300, .odds= 50 }  /* オレンのみ */,
        { .item_no= 17, .need_step=    0, .odds=100 }  /* キズぐすり */
    }
  },
  /* ---- Course 2: ざわざわもり ---- */
  { .graphic_id=2, .course_id=2, .name_string="ざわざわもり",
    .enemies = {
        { .species=202, .level=15, .held_item=  0, .moves={68, 243, 219, 194}, .form=0, .sex=1, .need_step= 4000, .odds= 30 }  /* ソーナンス (カウンター/ミラーコート/しんぴのまもり/みちづれ) */,
        { .species= 48, .level= 6, .held_item=  0, .moves={33, 50, 193, 48}, .form=0, .sex=0, .need_step=  700, .odds= 89 }  /* コンパン (たいあたり/かなしばり/みやぶる/ちょうおんぱ) */,
        { .species= 43, .level= 5, .held_item=  0, .moves={71, 230, 0, 0}, .form=0, .sex=1, .need_step=    0, .odds=100 }  /* ナゾノクサ (すいとる/あまいかおり/なし/なし) */
    },
    .items = {
        { .item_no=  6, .need_step= 5000, .odds= 30 }  /* ネットボール */,
        { .item_no= 28, .need_step= 2500, .odds= 20 }  /* げんきのかけら */,
        { .item_no= 75, .need_step= 2000, .odds= 20 }  /* みどりのかけら */,
        { .item_no= 38, .need_step= 1000, .odds=  5 }  /* ピーピーエイド */,
        { .item_no= 35, .need_step=  900, .odds= 10 }  /* ちからのねっこ */,
        { .item_no= 34, .need_step=  800, .odds= 50 }  /* ちからのこな */,
        { .item_no= 87, .need_step=  700, .odds= 10 }  /* おおきなキノコ */,
        { .item_no=151, .need_step=  500, .odds= 50 }  /* モモンのみ */,
        { .item_no= 86, .need_step=  200, .odds= 50 }  /* ちいさなキノコ */,
        { .item_no=149, .need_step=    0, .odds=100 }  /* クラボのみ */
    }
  },
  /* ---- Course 3: ごつごつみち ---- */
  { .graphic_id=5, .course_id=3, .name_string="ごつごつみち",
    .enemies = {
        { .species=240, .level= 9, .held_item=  0, .moves={123, 43, 52, 241}, .form=0, .sex=0, .need_step= 5000, .odds= 50 }  /* ブビィ (スモッグ/にらみつける/ひのこ/にほんばれ) */,
        { .species= 66, .level= 7, .held_item=  0, .moves={67, 43, 116, 0}, .form=0, .sex=1, .need_step= 1000, .odds= 92 }  /* ワンリキー (けたぐり/にらみつける/きあいだめ/なし) */,
        { .species=163, .level= 6, .held_item=  0, .moves={33, 45, 193, 95}, .form=0, .sex=1, .need_step=    0, .odds=100 }  /* ホーホー (たいあたり/なきごえ/みやぶる/さいみんじゅつ) */
    },
    .items = {
        { .item_no= 51, .need_step= 7000, .odds=  3 }  /* ポイントアップ */,
        { .item_no=238, .need_step= 5000, .odds= 10 }  /* かたいいし */,
        { .item_no= 72, .need_step= 3000, .odds= 20 }  /* あかいかけら */,
        { .item_no= 91, .need_step= 2000, .odds= 20 }  /* ほしのかけら */,
        { .item_no= 27, .need_step= 1500, .odds= 20 }  /* なんでもなおし */,
        { .item_no= 76, .need_step= 1000, .odds= 20 }  /* シルバースプレー */,
        { .item_no= 19, .need_step=  800, .odds= 20 }  /* やけどなおし */,
        { .item_no= 18, .need_step=  500, .odds= 40 }  /* どくけし */,
        { .item_no= 78, .need_step=  100, .odds= 50 }  /* あなぬけのヒモ */,
        { .item_no= 79, .need_step=    0, .odds=100 }  /* むしよけスプレー */
    }
  },
  /* ---- Course 4: きれいなうみべ ---- */
  { .graphic_id=8, .course_id=4, .name_string="きれいなうみべ",
    .enemies = {
        { .species= 54, .level=10, .held_item=  0, .moves={346, 10, 39, 55}, .form=0, .sex=1, .need_step= 4000, .odds= 70 }  /* コダック (みずあそび/ひっかく/しっぽをふる/みずでっぽう) */,
        { .species= 79, .level= 8, .held_item=  0, .moves={174, 281, 33, 45}, .form=0, .sex=0, .need_step= 1000, .odds= 87 }  /* ヤドン (のろい/あくび/たいあたり/なきごえ) */,
        { .species=191, .level= 6, .held_item=  0, .moves={71, 74, 72, 0}, .form=0, .sex=1, .need_step=    0, .odds=100 }  /* ヒマナッツ (すいとる/せいちょう/メガドレイン/なし) */
    },
    .items = {
        { .item_no=  7, .need_step= 5000, .odds= 30 }  /* ダイブボール */,
        { .item_no= 89, .need_step= 4000, .odds= 20 }  /* おおきなしんじゅ */,
        { .item_no= 73, .need_step= 3000, .odds= 20 }  /* あおいかけら */,
        { .item_no= 93, .need_step= 2000, .odds= 20 }  /* ハートのウロコ */,
        { .item_no=154, .need_step= 1800, .odds= 20 }  /* ヒメリのみ */,
        { .item_no= 27, .need_step= 1500, .odds= 30 }  /* なんでもなおし */,
        { .item_no=153, .need_step= 1000, .odds= 20 }  /* ナナシのみ */,
        { .item_no= 31, .need_step=  800, .odds= 50 }  /* サイコソーダ */,
        { .item_no=152, .need_step=  100, .odds= 40 }  /* チーゴのみ */,
        { .item_no= 30, .need_step=    0, .odds=100 }  /* おいしいみず */
    }
  },
  /* ---- Course 5: じゅうたくち ---- */
  { .graphic_id=3, .course_id=5, .name_string="じゅうたくち",
    .enemies = {
        { .species=239, .level=11, .held_item=  0, .moves={98, 43, 67, 9}, .form=0, .sex=0, .need_step= 5000, .odds= 15 }  /* エレキッド (でんこうせっか/にらみつける/けたぐり/かみなりパンチ) */,
        { .species= 81, .level= 8, .held_item=  0, .moves={319, 33, 84, 0}, .form=0, .sex=2, .need_step= 1000, .odds= 85 }  /* コイル (きんぞくおん/たいあたり/でんきショック/なし) */,
        { .species=163, .level= 7, .held_item=  0, .moves={33, 45, 193, 95}, .form=0, .sex=1, .need_step=    0, .odds=100 }  /* ホーホー (たいあたり/なきごえ/みやぶる/さいみんじゅつ) */
    },
    .items = {
        { .item_no= 51, .need_step= 5000, .odds=  3 }  /* ポイントアップ */,
        { .item_no= 55, .need_step= 2000, .odds= 30 }  /* エフェクトガード */,
        { .item_no= 62, .need_step= 1500, .odds= 30 }  /* スペシャルガード */,
        { .item_no= 61, .need_step= 1250, .odds= 30 }  /* スペシャルアップ */,
        { .item_no= 56, .need_step= 1000, .odds= 30 }  /* クリティカッター */,
        { .item_no= 60, .need_step=  750, .odds= 30 }  /* ヨクアタール */,
        { .item_no= 59, .need_step=  500, .odds= 30 }  /* スピーダー */,
        { .item_no= 58, .need_step=  250, .odds= 30 }  /* ディフェンダー */,
        { .item_no= 57, .need_step=  100, .odds= 30 }  /* プラスパワー */,
        { .item_no= 17, .need_step=    0, .odds=100 }  /* キズぐすり */
    }
  },
  /* ---- Course 6: くらいどうくつ ---- */
  { .graphic_id=6, .course_id=6, .name_string="くらいどうくつ",
    .enemies = {
        { .species=238, .level=12, .held_item=  0, .moves={1, 122, 186, 419}, .form=0, .sex=1, .need_step= 5000, .odds= 50 }  /* ムチュール (はたく/したでなめる/てんしのキッス/ゆきなだれ) */,
        { .species= 92, .level=10, .held_item=  0, .moves={95, 122, 180, 212}, .form=0, .sex=1, .need_step= 1000, .odds= 92 }  /* ゴース (さいみんじゅつ/したでなめる/うらみ/くろいまなざし) */,
        { .species= 41, .level= 8, .held_item=  0, .moves={141, 48, 0, 0}, .form=0, .sex=0, .need_step=    0, .odds=100 }  /* ズバット (きゅうけつ/ちょうおんぱ/なし/なし) */
    },
    .items = {
        { .item_no=345, .need_step= 6000, .odds= 20 }  /* わざマシン１８ */,
        { .item_no=222, .need_step= 5000, .odds=  5 }  /* ぎんのこな */,
        { .item_no= 74, .need_step= 4500, .odds= 20 }  /* きいろいかけら */,
        { .item_no= 40, .need_step= 3000, .odds=  5 }  /* ピーピーエイダー */,
        { .item_no=156, .need_step= 2500, .odds= 20 }  /* キーのみ */,
        { .item_no= 24, .need_step= 2000, .odds= 10 }  /* まんたんのくすり */,
        { .item_no= 39, .need_step= 1500, .odds=  5 }  /* ピーピーリカバー */,
        { .item_no= 25, .need_step= 1000, .odds= 20 }  /* すごいキズぐすり */,
        { .item_no= 38, .need_step=  500, .odds= 20 }  /* ピーピーエイド */,
        { .item_no=158, .need_step=    0, .odds=100 }  /* オボンのみ */
    }
  },
  /* ---- Course 7: あおいみずうみ ---- */
  { .graphic_id=7, .course_id=7, .name_string="あおいみずうみ",
    .enemies = {
        { .species=147, .level=10, .held_item=  0, .moves={35, 43, 86, 0}, .form=0, .sex=1, .need_step= 5000, .odds= 30 }  /* ミニリュウ (まきつく/にらみつける/でんじは/なし) */,
        { .species= 98, .level=12, .held_item=  0, .moves={11, 43, 106, 152}, .form=0, .sex=0, .need_step=  500, .odds= 72 }  /* クラブ (はさむ/にらみつける/かたくなる/クラブハンマー) */,
        { .species=118, .level= 9, .held_item=  0, .moves={64, 39, 346, 48}, .form=0, .sex=1, .need_step=    0, .odds=100 }  /* トサキント (つつく/しっぽをふる/みずあそび/ちょうおんぱ) */
    },
    .items = {
        { .item_no=338, .need_step= 5000, .odds= 20 }  /* わざマシン１１ */,
        { .item_no=  6, .need_step= 4000, .odds= 10 }  /* ネットボール */,
        { .item_no=  7, .need_step= 3500, .odds= 10 }  /* ダイブボール */,
        { .item_no=157, .need_step= 3000, .odds= 15 }  /* ラムのみ */,
        { .item_no= 91, .need_step= 2500, .odds=  5 }  /* ほしのかけら */,
        { .item_no= 90, .need_step= 2000, .odds= 20 }  /* ほしのすな */,
        { .item_no=158, .need_step= 1000, .odds= 20 }  /* オボンのみ */,
        { .item_no= 88, .need_step=  500, .odds=  5 }  /* しんじゅ */,
        { .item_no=154, .need_step=  100, .odds= 20 }  /* ヒメリのみ */,
        { .item_no= 30, .need_step=    0, .odds=100 }  /* おいしいみず */
    }
  },
  /* ---- Course 8: まちのはずれ ---- */
  { .graphic_id=4, .course_id=8, .name_string="まちのはずれ",
    .enemies = {
        { .species= 63, .level=15, .held_item=  0, .moves={100, 0, 0, 0}, .form=0, .sex=1, .need_step= 5000, .odds= 40 }  /* ケーシィ (テレポート/なし/なし/なし) */,
        { .species=109, .level=13, .held_item=  0, .moves={33, 123, 108, 120}, .form=0, .sex=1, .need_step= 1500, .odds= 75 }  /* ドガース (たいあたり/スモッグ/えんまく/じばく) */,
        { .species= 19, .level=16, .held_item=  0, .moves={116, 44, 228, 158}, .form=0, .sex=1, .need_step=    0, .odds=100 }  /* コラッタ (きあいだめ/かみつく/おいうち/ひっさつまえば) */
    },
    .items = {
        { .item_no=364, .need_step= 5000, .odds= 20 }  /* わざマシン３７ */,
        { .item_no= 55, .need_step= 3000, .odds= 20 }  /* エフェクトガード */,
        { .item_no= 62, .need_step= 2500, .odds= 10 }  /* スペシャルガード */,
        { .item_no=  2, .need_step= 2000, .odds= 20 }  /* ハイパーボール */,
        { .item_no=157, .need_step= 1500, .odds= 20 }  /* ラムのみ */,
        { .item_no= 57, .need_step= 1000, .odds= 20 }  /* プラスパワー */,
        { .item_no=  3, .need_step=  750, .odds= 10 }  /* スーパーボール */,
        { .item_no= 60, .need_step=  500, .odds= 20 }  /* ヨクアタール */,
        { .item_no= 56, .need_step=  100, .odds= 20 }  /* クリティカッター */,
        { .item_no=  4, .need_step=    0, .odds=100 }  /* モンスターボール */
    }
  },
  /* ---- Course 9: ホウエンのはら ---- */
  { .graphic_id=1, .course_id=9, .name_string="ホウエンのはら",
    .enemies = {
        { .species=300, .level=30, .held_item=  0, .moves={213, 274, 204, 185}, .form=0, .sex=1, .need_step= 7500, .odds= 50 }  /* エネコ (メロメロ/ねこのて/あまえる/だましうち) */,
        { .species=314, .level=25, .held_item=  0, .moves={204, 236, 273, 227}, .form=0, .sex=1, .need_step= 2000, .odds= 84 }  /* イルミーゼ (あまえる/つきのひかり/ねがいごと/アンコール) */,
        { .species=263, .level=17, .held_item=  0, .moves={39, 29, 28, 316}, .form=0, .sex=1, .need_step=    0, .odds=100 }  /* ジグザグマ (しっぽをふる/ずつき/すなかけ/かぎわける) */
    },
    .items = {
        { .item_no=202, .need_step= 8000, .odds=  5 }  /* リュガのみ */,
        { .item_no=186, .need_step= 3500, .odds= 10 }  /* ソクノのみ */,
        { .item_no=185, .need_step= 3000, .odds= 10 }  /* イトケのみ */,
        { .item_no=184, .need_step= 2500, .odds= 20 }  /* オッカのみ */,
        { .item_no=171, .need_step= 2000, .odds= 20 }  /* タポルのみ */,
        { .item_no=170, .need_step= 1500, .odds= 20 }  /* ネコブのみ */,
        { .item_no=169, .need_step= 1000, .odds= 20 }  /* ザロクのみ */,
        { .item_no=166, .need_step=  500, .odds= 20 }  /* ナナのみ */,
        { .item_no=165, .need_step=  250, .odds= 20 }  /* ブリーのみ */,
        { .item_no=164, .need_step=    0, .odds=100 }  /* ズリのみ */
    }
  },
  /* ---- Course 10: あったかビーチ ---- */
  { .graphic_id=8, .course_id=10, .name_string="あったかビーチ",
    .enemies = {
        { .species=320, .level=31, .held_item=  0, .moves={352, 54, 156, 362}, .form=0, .sex=1, .need_step= 7000, .odds= 50 }  /* ホエルコ (みずのはどう/しろいきり/ねむる/しおみず) */,
        { .species=116, .level=20, .held_item=  0, .moves={43, 55, 116, 61}, .form=0, .sex=1, .need_step= 1500, .odds= 84 }  /* タッツー (にらみつける/みずでっぽう/きあいだめ/バブルこうせん) */,
        { .species=118, .level=22, .held_item=  0, .moves={346, 48, 30, 401}, .form=0, .sex=1, .need_step=    0, .odds=100 }  /* トサキント (みずあそび/ちょうおんぱ/つのでつく/アクアテール) */
    },
    .items = {
        { .item_no=201, .need_step= 8000, .odds=  5 }  /* チイラのみ */,
        { .item_no= 82, .need_step= 5000, .odds=  5 }  /* ほのおのいし */,
        { .item_no= 74, .need_step= 4900, .odds= 20 }  /* きいろいかけら */,
        { .item_no= 73, .need_step= 4000, .odds= 20 }  /* あおいかけら */,
        { .item_no= 93, .need_step= 3000, .odds=  5 }  /* ハートのウロコ */,
        { .item_no= 89, .need_step= 2500, .odds=  5 }  /* おおきなしんじゅ */,
        { .item_no= 28, .need_step= 2000, .odds= 40 }  /* げんきのかけら */,
        { .item_no= 88, .need_step= 1000, .odds= 10 }  /* しんじゅ */,
        { .item_no=167, .need_step=  100, .odds= 20 }  /* セシナのみ */,
        { .item_no= 30, .need_step=    0, .odds=100 }  /* おいしいみず */
    }
  },
  /* ---- Course 11: かざんどうくつ ---- */
  { .graphic_id=5, .course_id=11, .name_string="かざんどうくつ",
    .enemies = {
        { .species=218, .level=31, .held_item=  0, .moves={52, 105, 246, 133}, .form=0, .sex=1, .need_step= 5000, .odds= 70 }  /* マグマッグ (ひのこ/じこさいせい/げんしのちから/ドわすれ) */,
        { .species=228, .level=27, .held_item=  0, .moves={46, 44, 316, 251}, .form=0, .sex=0, .need_step= 2000, .odds= 85 }  /* デルビル (ほえる/かみつく/かぎわける/ふくろだたき) */,
        { .species= 77, .level=19, .held_item=  0, .moves={39, 52, 172, 23}, .form=0, .sex=1, .need_step=    0, .odds=100 }  /* ポニータ (しっぽをふる/ひのこ/かえんぐるま/ふみつけ) */
    },
    .items = {
        { .item_no=205, .need_step= 8000, .odds=  5 }  /* ズアのみ */,
        { .item_no= 80, .need_step= 5500, .odds=  5 }  /* たいようのいし */,
        { .item_no= 81, .need_step= 5000, .odds= 20 }  /* つきのいし */,
        { .item_no=273, .need_step= 4000, .odds=  5 }  /* かえんだま */,
        { .item_no= 42, .need_step= 3000, .odds=  5 }  /* フエンせんべい */,
        { .item_no= 28, .need_step= 2000, .odds= 30 }  /* げんきのかけら */,
        { .item_no=168, .need_step= 1000, .odds= 20 }  /* パイルのみ */,
        { .item_no= 72, .need_step=  500, .odds= 20 }  /* あかいかけら */,
        { .item_no= 20, .need_step=  100, .odds= 50 }  /* こおりなおし */,
        { .item_no= 17, .need_step=    0, .odds=100 }  /* キズぐすり */
    }
  },
  /* ---- Course 12: ツリーハウス ---- */
  { .graphic_id=2, .course_id=12, .name_string="ツリーハウス",
    .enemies = {
        { .species=352, .level=30, .held_item=  0, .moves={364, 60, 425, 163}, .form=0, .sex=0, .need_step= 5000, .odds= 30 }  /* カクレオン (フェイント/サイケこうせん/かげうち/きりさく) */,
        { .species=203, .level=28, .held_item=  0, .moves={97, 60, 226, 372}, .form=0, .sex=1, .need_step= 1000, .odds= 85 }  /* キリンリキ (こうそくいどう/サイケこうせん/バトンタッチ/ダメおし) */,
        { .species= 44, .level=14, .held_item=  0, .moves={71, 230, 51, 77}, .form=0, .sex=1, .need_step=    0, .odds=100 }  /* クサイハナ (すいとる/あまいかおり/ようかいえき/どくのこな) */
    },
    .items = {
        { .item_no=203, .need_step= 8000, .odds=  5 }  /* カムラのみ */,
        { .item_no= 49, .need_step= 4500, .odds= 10 }  /* リゾチウム */,
        { .item_no= 41, .need_step= 4000, .odds=  3 }  /* ピーピーマックス */,
        { .item_no= 39, .need_step= 3000, .odds=  5 }  /* ピーピーリカバー */,
        { .item_no= 24, .need_step= 3500, .odds= 20 }  /* まんたんのくすり */,
        { .item_no= 38, .need_step= 2000, .odds=  5 }  /* ピーピーエイド */,
        { .item_no= 37, .need_step= 1000, .odds= 20 }  /* ふっかつそう */,
        { .item_no= 36, .need_step=  500, .odds= 20 }  /* ばんのうごな */,
        { .item_no= 35, .need_step=  100, .odds= 20 }  /* ちからのねっこ */,
        { .item_no= 34, .need_step=    0, .odds=100 }  /* ちからのこな */
    }
  },
  /* ---- Course 13: こわいどうくつ ---- */
  { .graphic_id=6, .course_id=13, .name_string="こわいどうくつ",
    .enemies = {
        { .species=105, .level=30, .held_item=  0, .moves={155, 99, 206, 37}, .form=0, .sex=1, .need_step= 5000, .odds= 45 }  /* ガラガラ (ホネブーメラン/いかり/みねうち/あばれる) */,
        { .species= 42, .level=33, .held_item=  0, .moves={17, 109, 314, 212}, .form=0, .sex=0, .need_step=  500, .odds= 65 }  /* ゴルバット (つばさでうつ/あやしいひかり/エアカッター/くろいまなざし) */,
        { .species= 66, .level=13, .held_item=  0, .moves={43, 116, 2, 418}, .form=0, .sex=0, .need_step=    0, .odds=100 }  /* ワンリキー (にらみつける/きあいだめ/からてチョップ/バレットパンチ) */
    },
    .items = {
        { .item_no=204, .need_step= 8000, .odds=  5 }  /* ヤタピのみ */,
        { .item_no= 47, .need_step= 4900, .odds=  5 }  /* ブロムヘキシン */,
        { .item_no=258, .need_step= 4500, .odds=  5 }  /* ふといホネ */,
        { .item_no= 13, .need_step= 4000, .odds= 20 }  /* ダークボール */,
        { .item_no= 73, .need_step= 3900, .odds= 20 }  /* あおいかけら */,
        { .item_no= 54, .need_step= 3500, .odds=  5 }  /* もりのヨウカン */,
        { .item_no= 75, .need_step= 2000, .odds= 20 }  /* みどりのかけら */,
        { .item_no= 74, .need_step= 1500, .odds= 20 }  /* きいろいかけら */,
        { .item_no= 72, .need_step= 1000, .odds= 20 }  /* あかいかけら */,
        { .item_no= 79, .need_step=    0, .odds=100 }  /* むしよけスプレー */
    }
  },
  /* ---- Course 14: シンオウのはら ---- */
  { .graphic_id=1, .course_id=14, .name_string="シンオウのはら",
    .enemies = {
        { .species=439, .level=29, .held_item=  0, .moves={113, 115, 60, 164}, .form=0, .sex=0, .need_step= 7000, .odds= 40 }  /* マネネ (ひかりのかべ/リフレクター/サイケこうせん/みがわり) */,
        { .species=403, .level=33, .held_item=  0, .moves={46, 207, 422, 242}, .form=0, .sex=1, .need_step= 3000, .odds= 55 }  /* コリンク (ほえる/いばる/かみなりのキバ/かみくだく) */,
        { .species=399, .level=13, .held_item=  0, .moves={33, 45, 111, 205}, .form=0, .sex=1, .need_step=    0, .odds=100 }  /* ビッパ (たいあたり/なきごえ/まるくなる/ころがる) */
    },
    .items = {
        { .item_no= 50, .need_step= 5000, .odds= 10 }  /* ふしぎなアメ */,
        { .item_no= 23, .need_step= 4000, .odds= 10 }  /* かいふくのくすり */,
        { .item_no= 24, .need_step= 3800, .odds= 20 }  /* まんたんのくすり */,
        { .item_no=  9, .need_step= 3000, .odds= 20 }  /* リピートボール */,
        { .item_no= 10, .need_step= 2500, .odds= 20 }  /* タイマーボール */,
        { .item_no= 28, .need_step= 2000, .odds= 20 }  /* げんきのかけら */,
        { .item_no= 25, .need_step= 1500, .odds= 20 }  /* すごいキズぐすり */,
        { .item_no= 27, .need_step= 1000, .odds= 20 }  /* なんでもなおし */,
        { .item_no=  8, .need_step=  500, .odds= 20 }  /* ネストボール */,
        { .item_no= 26, .need_step=    0, .odds=100 }  /* いいキズぐすり */
    }
  },
  /* ---- Course 15: さむいやまみち ---- */
  { .graphic_id=5, .course_id=15, .name_string="さむいやまみち",
    .enemies = {
        { .species=459, .level=31, .held_item=  0, .moves={452, 54, 420, 275}, .form=0, .sex=0, .need_step=10000, .odds= 50 }  /* ユキカブリ (ウッドハンマー/しろいきり/こおりのつぶて/ねをはる) */,
        { .species=215, .level=28, .held_item=  0, .moves={185, 306, 97, 196}, .form=0, .sex=0, .need_step= 3000, .odds= 55 }  /* ニューラ (だましうち/ブレイククロー/こうそくいどう/こごえるかぜ) */,
        { .species=220, .level=16, .held_item=  0, .moves={300, 181, 189, 203}, .form=0, .sex=1, .need_step=    0, .odds=100 }  /* ウリムー (どろあそび/こなゆき/どろかけ/こらえる) */
    },
    .items = {
        { .item_no=334, .need_step= 6000, .odds= 20 }  /* わざマシン０７ */,
        { .item_no=188, .need_step= 5000, .odds= 10 }  /* ヤチェのみ */,
        { .item_no=187, .need_step= 4500, .odds= 10 }  /* リンドのみ */,
        { .item_no=282, .need_step= 4000, .odds=  5 }  /* つめたいいわ */,
        { .item_no=283, .need_step= 3500, .odds=  5 }  /* さらさらいわ */,
        { .item_no=284, .need_step= 3000, .odds=  5 }  /* あついいわ */,
        { .item_no=285, .need_step= 2500, .odds=  5 }  /* しめったいわ */,
        { .item_no= 77, .need_step= 1000, .odds= 30 }  /* ゴールドスプレー */,
        { .item_no= 27, .need_step=  100, .odds= 40 }  /* なんでもなおし */,
        { .item_no= 58, .need_step=    0, .odds=100 }  /* ディフェンダー */
    }
  },
  /* ---- Course 16: おおきなもり ---- */
  { .graphic_id=2, .course_id=16, .name_string="おおきなもり",
    .enemies = {
        { .species=357, .level=35, .held_item=  0, .moves={23, 230, 18, 345}, .form=0, .sex=1, .need_step= 6000, .odds= 50 }  /* トロピウス (ふみつけ/あまいかおり/ふきとばし/マジカルリーフ) */,
        { .species=114, .level=30, .held_item=  0, .moves={22, 20, 72, 78}, .form=0, .sex=1, .need_step= 1000, .odds= 55 }  /* モンジャラ (つるのムチ/しめつける/メガドレイン/しびれごな) */,
        { .species=179, .level=19, .held_item=  0, .moves={45, 84, 86, 178}, .form=0, .sex=0, .need_step=    0, .odds=100 }  /* メリープ (なきごえ/でんきショック/でんじは/わたほうし) */
    },
    .items = {
        { .item_no=172, .need_step= 5000, .odds= 10 }  /* ロメのみ */,
        { .item_no=171, .need_step= 4500, .odds= 10 }  /* タポルのみ */,
        { .item_no=165, .need_step= 4000, .odds= 10 }  /* ブリーのみ */,
        { .item_no=182, .need_step= 3500, .odds= 20 }  /* ドリのみ */,
        { .item_no= 87, .need_step= 3000, .odds= 50 }  /* おおきなキノコ */,
        { .item_no=183, .need_step= 2500, .odds= 20 }  /* ベリブのみ */,
        { .item_no= 94, .need_step= 2000, .odds= 20 }  /* あまいミツ */,
        { .item_no=174, .need_step= 1000, .odds= 20 }  /* マトマのみ */,
        { .item_no=173, .need_step=  500, .odds= 20 }  /* ウブのみ */,
        { .item_no= 86, .need_step=    0, .odds=100 }  /* ちいさなキノコ */
    }
  },
  /* ---- Course 17: しろいみずうみ ---- */
  { .graphic_id=7, .course_id=17, .name_string="しろいみずうみ",
    .enemies = {
        { .species=433, .level=22, .held_item=  0, .moves={310, 105, 253, 387}, .form=0, .sex=0, .need_step= 5000, .odds= 50 }  /* リーシャン (おどろかす/じこさいせい/さわぐ/とっておき) */,
        { .species= 93, .level=25, .held_item=  0, .moves={101, 109, 389, 325}, .form=0, .sex=0, .need_step=  500, .odds= 55 }  /* ゴースト (ナイトヘッド/あやしいひかり/ふいうち/シャドーパンチ) */,
        { .species=223, .level=19, .held_item=  0, .moves={199, 60, 62, 61}, .form=0, .sex=1, .need_step=    0, .odds=100 }  /* テッポウオ (ロックオン/サイケこうせん/オーロラビーム/バブルこうせん) */
    },
    .items = {
        { .item_no=395, .need_step=10000, .odds=  1 }  /* わざマシン６８ */,
        { .item_no=190, .need_step= 5000, .odds= 10 }  /* ビアーのみ */,
        { .item_no=189, .need_step= 4500, .odds= 10 }  /* ヨプのみ */,
        { .item_no=181, .need_step= 4000, .odds= 20 }  /* カイスのみ */,
        { .item_no=180, .need_step= 3500, .odds= 20 }  /* シーヤのみ */,
        { .item_no=179, .need_step= 3000, .odds= 20 }  /* ノワキのみ */,
        { .item_no=178, .need_step= 2500, .odds= 20 }  /* ノメルのみ */,
        { .item_no=177, .need_step= 2000, .odds= 20 }  /* ラブタのみ */,
        { .item_no=176, .need_step= 1000, .odds= 20 }  /* ゴスのみ */,
        { .item_no=175, .need_step=    0, .odds=100 }  /* モコシのみ */
    }
  },
  /* ---- Course 18: あれたうみべ ---- */
  { .graphic_id=8, .course_id=18, .name_string="あれたうみべ",
    .enemies = {
        { .species=456, .level=26, .held_item=  0, .moves={240, 16, 352, 445}, .form=0, .sex=1, .need_step= 4000, .odds= 30 }  /* ケイコウオ (あまごい/かぜおこし/みずのはどう/ゆうわく) */,
        { .species=129, .level=30, .held_item=  0, .moves={150, 33, 175, 340}, .form=0, .sex=1, .need_step=  500, .odds= 65 }  /* コイキング (はねる/たいあたり/じたばた/とびはねる) */,
        { .species= 54, .level=22, .held_item=  0, .moves={8, 50, 93, 281}, .form=0, .sex=1, .need_step=    0, .odds=100 }  /* コダック (れいとうパンチ/かなしばり/ねんりき/あくび) */
    },
    .items = {
        { .item_no= 51, .need_step= 5000, .odds= 10 }  /* ポイントアップ */,
        { .item_no=193, .need_step= 4500, .odds= 10 }  /* ウタンのみ */,
        { .item_no=192, .need_step= 4000, .odds= 10 }  /* バコウのみ */,
        { .item_no=191, .need_step= 3500, .odds= 10 }  /* シュカのみ */,
        { .item_no=194, .need_step= 3000, .odds= 10 }  /* タンガのみ */,
        { .item_no=195, .need_step= 2500, .odds= 10 }  /* ヨロギのみ */,
        { .item_no= 14, .need_step= 2000, .odds= 20 }  /* ヒールボール */,
        { .item_no= 15, .need_step= 1500, .odds= 20 }  /* クイックボール */,
        { .item_no= 93, .need_step=  800, .odds= 40 }  /* ハートのウロコ */,
        { .item_no= 30, .need_step=    0, .odds=100 }  /* おいしいみず */
    }
  },
  /* ---- Course 19: リゾートち ---- */
  { .graphic_id=3, .course_id=19, .name_string="リゾートち",
    .enemies = {
        { .species=417, .level=33, .held_item=  0, .moves={175, 186, 435, 162}, .form=0, .sex=1, .need_step= 8000, .odds= 45 }  /* パチリス (じたばた/てんしのキッス/ほうでん/いかりのまえば) */,
        { .species= 39, .level=30, .held_item=  0, .moves={205, 3, 156, 34}, .form=0, .sex=1, .need_step= 4000, .odds= 55 }  /* プリン (ころがる/おうふくビンタ/ねむる/のしかかり) */,
        { .species=183, .level=25, .held_item=  0, .moves={55, 205, 61, 392}, .form=0, .sex=1, .need_step=    0, .odds=100 }  /* マリル (みずでっぽう/ころがる/バブルこうせん/アクアリング) */
    },
    .items = {
        { .item_no= 11, .need_step= 6000, .odds= 10 }  /* ゴージャスボール */,
        { .item_no= 63, .need_step= 5000, .odds= 10 }  /* ピッピにんぎょう */,
        { .item_no= 64, .need_step= 3000, .odds= 20 }  /* エネコのシッポ */,
        { .item_no= 33, .need_step= 2500, .odds= 20 }  /* モーモーミルク */,
        { .item_no= 94, .need_step= 2000, .odds=  5 }  /* あまいミツ */,
        { .item_no= 31, .need_step= 1000, .odds= 20 }  /* サイコソーダ */,
        { .item_no= 77, .need_step=  800, .odds=  5 }  /* ゴールドスプレー */,
        { .item_no= 32, .need_step=  500, .odds=  5 }  /* ミックスオレ */,
        { .item_no= 75, .need_step=  200, .odds= 20 }  /* みどりのかけら */,
        { .item_no= 30, .need_step=    0, .odds=100 }  /* おいしいみず */
    }
  },
  /* ---- Course 20: しずかどうくつ ---- */
  { .graphic_id=6, .course_id=20, .name_string="しずかどうくつ",
    .enemies = {
        { .species=442, .level=31, .held_item=  0, .moves={95, 138, 466, 389}, .form=0, .sex=0, .need_step=10000, .odds=  5 }  /* ミカルゲ (さいみんじゅつ/ゆめくい/あやしいかぜ/ふいうち) */,
        { .species=433, .level=26, .held_item=  0, .moves={310, 93, 253, 387}, .form=0, .sex=1, .need_step=  500, .odds= 45 }  /* リーシャン (おどろかす/ねんりき/さわぐ/とっておき) */,
        { .species=164, .level=30, .held_item=  0, .moves={95, 115, 93, 36}, .form=0, .sex=1, .need_step=    0, .odds=100 }  /* ヨルノズク (さいみんじゅつ/リフレクター/ねんりき/とっしん) */
    },
    .items = {
        { .item_no=342, .need_step=10000, .odds=  1 }  /* わざマシン１５ */,
        { .item_no= 92, .need_step= 5500, .odds=  5 }  /* きんのたま */,
        { .item_no=337, .need_step= 5000, .odds= 20 }  /* わざマシン１０ */,
        { .item_no= 41, .need_step= 4500, .odds=  5 }  /* ピーピーマックス */,
        { .item_no= 89, .need_step= 3000, .odds= 10 }  /* おおきなしんじゅ */,
        { .item_no= 93, .need_step= 2500, .odds= 30 }  /* ハートのウロコ */,
        { .item_no= 13, .need_step= 2000, .odds= 20 }  /* ダークボール */,
        { .item_no= 90, .need_step= 1000, .odds= 20 }  /* ほしのすな */,
        { .item_no= 88, .need_step=  500, .odds= 20 }  /* しんじゅ */,
        { .item_no= 40, .need_step=    0, .odds=100 }  /* ピーピーエイダー */
    }
  },
  /* ---- Course 21: うみのむこう ---- */
  { .graphic_id=8, .course_id=21, .name_string="うみのむこう",
    .enemies = {
        { .species=120, .level=18, .held_item= 84, .moves={55, 229, 105, 113}, .form=0, .sex=0, .need_step= 5000, .odds= 20 }  /* ヒトデマン (みずでっぽう/こうそくスピン/じこさいせい/ひかりのかべ) */,
        { .species=116, .level=15, .held_item=  0, .moves={108, 43, 55, 116}, .form=0, .sex=0, .need_step= 2500, .odds= 55 }  /* タッツー (えんまく/にらみつける/みずでっぽう/きあいだめ) */,
        { .species=223, .level=14, .held_item=  0, .moves={55, 199, 60, 62}, .form=0, .sex=0, .need_step=    0, .odds=100 }  /* テッポウオ (みずでっぽう/ロックオン/サイケこうせん/オーロラビーム) */
    },
    .items = {
        { .item_no=200, .need_step= 5000, .odds= 20 }  /* ホズのみ */,
        { .item_no=199, .need_step= 4500, .odds= 20 }  /* リリバのみ */,
        { .item_no=198, .need_step= 4000, .odds= 20 }  /* ナモのみ */,
        { .item_no=197, .need_step= 3500, .odds= 20 }  /* ハバンのみ */,
        { .item_no=196, .need_step= 3000, .odds= 20 }  /* カシブのみ */,
        { .item_no=170, .need_step= 2500, .odds= 20 }  /* ネコブのみ */,
        { .item_no=180, .need_step= 2000, .odds= 20 }  /* シーヤのみ */,
        { .item_no=179, .need_step= 1500, .odds= 20 }  /* ノワキのみ */,
        { .item_no=169, .need_step= 1000, .odds= 20 }  /* ザロクのみ */,
        { .item_no= 90, .need_step=    0, .odds=100 }  /* ほしのすな */
    }
  },
  /* ---- Course 22: よぞらのはて ---- */
  { .graphic_id=5, .course_id=22, .name_string="よぞらのはて",
    .enemies = {
        { .species= 35, .level= 8, .held_item=  0, .moves={1, 227, 47, 236}, .form=0, .sex=0, .need_step= 5000, .odds= 55 }  /* ピッピ (はたく/アンコール/うたう/つきのひかり) */,
        { .species= 41, .level= 9, .held_item=  0, .moves={141, 48, 310, 0}, .form=0, .sex=0, .need_step= 2500, .odds= 75 }  /* ズバット (きゅうけつ/ちょうおんぱ/おどろかす/なし) */,
        { .species= 74, .level= 5, .held_item=  0, .moves={33, 111, 300, 0}, .form=0, .sex=0, .need_step=    0, .odds=100 }  /* イシツブテ (たいあたり/まるくなる/どろあそび/なし) */
    },
    .items = {
        { .item_no=356, .need_step=10000, .odds=  1 }  /* わざマシン２９ */,
        { .item_no= 81, .need_step= 5000, .odds= 30 }  /* つきのいし */,
        { .item_no=106, .need_step= 4000, .odds= 20 }  /* きちょうなホネ */,
        { .item_no= 91, .need_step= 3500, .odds= 20 }  /* ほしのかけら */,
        { .item_no= 75, .need_step= 3000, .odds= 20 }  /* みどりのかけら */,
        { .item_no= 74, .need_step= 2500, .odds= 20 }  /* きいろいかけら */,
        { .item_no= 72, .need_step= 2000, .odds= 20 }  /* あかいかけら */,
        { .item_no= 73, .need_step= 1500, .odds= 20 }  /* あおいかけら */,
        { .item_no= 88, .need_step= 1000, .odds= 10 }  /* しんじゅ */,
        { .item_no= 90, .need_step=    0, .odds=100 }  /* ほしのすな */
    }
  },
  /* ---- Course 23: きいろのもり ---- */
  { .graphic_id=2, .course_id=23, .name_string="きいろのもり",
    .enemies = {
        { .species= 25, .level=15, .held_item=191, .moves={19, 87, 45, 39}, .form=0, .sex=0, .need_step=10000, .odds=  2 }  /* ピカチュウ (そらをとぶ/かみなり/なきごえ/しっぽをふる) */,
        { .species= 25, .level=13, .held_item=154, .moves={175, 270, 351, 86}, .form=0, .sex=0, .need_step= 2000, .odds= 35 }  /* ピカチュウ (じたばた/てだすけ/でんげきは/でんじは) */,
        { .species= 25, .level=10, .held_item= 86, .moves={84, 45, 39, 86}, .form=0, .sex=0, .need_step=    0, .odds=100 }  /* ピカチュウ (でんきショック/なきごえ/しっぽをふる/でんじは) */
    },
    .items = {
        { .item_no=236, .need_step= 7000, .odds=  3 }  /* でんきだま */,
        { .item_no= 83, .need_step= 6000, .odds=  5 }  /* かみなりのいし */,
        { .item_no=239, .need_step= 5000, .odds= 10 }  /* きせきのタネ */,
        { .item_no=296, .need_step= 4000, .odds= 10 }  /* おおきなねっこ */,
        { .item_no=153, .need_step= 1000, .odds= 40 }  /* ナナシのみ */,
        { .item_no=152, .need_step=  900, .odds= 40 }  /* チーゴのみ */,
        { .item_no=151, .need_step=  800, .odds= 50 }  /* モモンのみ */,
        { .item_no=150, .need_step=  700, .odds= 50 }  /* カゴのみ */,
        { .item_no=149, .need_step=  600, .odds= 50 }  /* クラボのみ */,
        { .item_no= 87, .need_step=    0, .odds=100 }  /* おおきなキノコ */
    }
  },
  /* ---- Course 24: イベント ---- */
  { .graphic_id=3, .course_id=24, .name_string="イベント",
    .enemies = {
        { .species=441, .level=15, .held_item=  0, .moves={64, 45, 119, 47}, .form=0, .sex=0, .need_step= 1000, .odds= 25 }  /* ペラップ (つつく/なきごえ/オウムがえし/うたう) */,
        { .species= 25, .level=10, .held_item=  0, .moves={84, 45, 39, 86}, .form=0, .sex=1, .need_step=  500, .odds= 55 }  /* ピカチュウ (でんきショック/なきごえ/しっぽをふる/でんじは) */,
        { .species=427, .level= 5, .held_item=  0, .moves={150, 1, 111, 193}, .form=0, .sex=1, .need_step=    0, .odds=100 }  /* ミミロル (はねる/はたく/まるくなる/みやぶる) */
    },
    .items = {
        { .item_no= 66, .need_step= 1000, .odds=  5 }  /* きいろビードロ */,
        { .item_no= 29, .need_step=  900, .odds= 10 }  /* げんきのかたまり */,
        { .item_no= 40, .need_step=  800, .odds=  5 }  /* ピーピーエイダー */,
        { .item_no= 88, .need_step=  700, .odds= 20 }  /* しんじゅ */,
        { .item_no= 77, .need_step=  500, .odds= 30 }  /* ゴールドスプレー */,
        { .item_no= 26, .need_step=  300, .odds= 70 }  /* いいキズぐすり */,
        { .item_no= 78, .need_step=  200, .odds= 40 }  /* あなぬけのヒモ */,
        { .item_no= 22, .need_step=  150, .odds= 40 }  /* まひなおし */,
        { .item_no= 18, .need_step=  100, .odds= 40 }  /* どくけし */,
        { .item_no= 17, .need_step=    0, .odds=100 }  /* キズぐすり */
    }
  },
  /* ---- Course 25: おかいもの ---- */
  { .graphic_id=4, .course_id=25, .name_string="おかいもの",
    .enemies = {
        { .species=255, .level=10, .held_item=  0, .moves={10, 45, 116, 52}, .form=0, .sex=0, .need_step=10000, .odds=  1 }  /* アチャモ (ひっかく/なきごえ/きあいだめ/ひのこ) */,
        { .species=279, .level=15, .held_item=  0, .moves={346, 17, 48, 55}, .form=0, .sex=0, .need_step= 3000, .odds= 35 }  /* ペリッパー (みずあそび/つばさでうつ/ちょうおんぱ/みずでっぽう) */,
        { .species= 52, .level=10, .held_item=  0, .moves={10, 45, 44, 252}, .form=0, .sex=0, .need_step=    0, .odds=100 }  /* ニャース (ひっかく/なきごえ/かみつく/ねこだまし) */
    },
    .items = {
        { .item_no= 50, .need_step= 5000, .odds= 10 }  /* ふしぎなアメ */,
        { .item_no= 54, .need_step= 4500, .odds= 10 }  /* もりのヨウカン */,
        { .item_no= 35, .need_step= 3500, .odds= 15 }  /* ちからのねっこ */,
        { .item_no= 34, .need_step= 3000, .odds= 15 }  /* ちからのこな */,
        { .item_no= 33, .need_step= 2600, .odds= 30 }  /* モーモーミルク */,
        { .item_no= 30, .need_step= 2200, .odds= 30 }  /* おいしいみず */,
        { .item_no= 31, .need_step= 1800, .odds= 35 }  /* サイコソーダ */,
        { .item_no= 32, .need_step= 1400, .odds= 35 }  /* ミックスオレ */,
        { .item_no= 42, .need_step= 1000, .odds= 50 }  /* フエンせんべい */,
        { .item_no= 94, .need_step=    0, .odds=100 }  /* あまいミツ */
    }
  },
  /* ---- Course 26: チャンプのみち ---- */
  { .graphic_id=5, .course_id=26, .name_string="チャンプのみち",
    .enemies = {
        { .species=446, .level= 5, .held_item=234, .moves={118, 33, 111, 120}, .form=0, .sex=0, .need_step= 8000, .odds=  5 }  /* ゴンベ (ゆびをふる/たいあたり/まるくなる/じばく) */,
        { .species=116, .level= 5, .held_item=235, .moves={145, 108, 330, 0}, .form=0, .sex=0, .need_step= 3000, .odds= 55 }  /* タッツー (あわ/えんまく/だくりゅう/なし) */,
        { .species=129, .level= 5, .held_item=186, .moves={150, 340, 0, 0}, .form=0, .sex=0, .need_step=    0, .odds=100 }  /* コイキング (はねる/とびはねる/なし/なし) */
    },
    .items = {
        { .item_no=275, .need_step=10000, .odds=  3 }  /* きあいのタスキ */,
        { .item_no=287, .need_step= 9000, .odds=  3 }  /* こだわりスカーフ */,
        { .item_no=220, .need_step= 8000, .odds=  3 }  /* こだわりハチマキ */,
        { .item_no=297, .need_step= 7000, .odds=  3 }  /* こだわりメガネ */,
        { .item_no=271, .need_step= 6000, .odds=  3 }  /* パワフルハーブ */,
        { .item_no=214, .need_step= 5000, .odds=  3 }  /* しろいハーブ */,
        { .item_no=158, .need_step= 2000, .odds= 20 }  /* オボンのみ */,
        { .item_no=157, .need_step= 1000, .odds= 20 }  /* ラムのみ */,
        { .item_no=156, .need_step=  500, .odds= 50 }  /* キーのみ */,
        { .item_no=150, .need_step=    0, .odds=100 }  /* カゴのみ */
    }
  },
  /* ---- Course 27: ふれあいのはら ---- */
  { .graphic_id=1, .course_id=27, .name_string="ふれあいのはら",
    .enemies = {
        { .species=239, .level= 5, .held_item=174, .moves={98, 43, 9, 0}, .form=0, .sex=0, .need_step= 5000, .odds= 20 }  /* エレキッド (でんこうせっか/にらみつける/かみなりパンチ/なし) */,
        { .species=238, .level= 5, .held_item=172, .moves={1, 122, 8, 0}, .form=0, .sex=1, .need_step= 2000, .odds= 55 }  /* ムチュール (はたく/したでなめる/れいとうパンチ/なし) */,
        { .species=174, .level= 5, .held_item=173, .moves={47, 204, 111, 273}, .form=0, .sex=0, .need_step=    0, .odds=100 }  /* ププリン (うたう/あまえる/まるくなる/ねがいごと) */
    },
    .items = {
        { .item_no=322, .need_step= 2550, .odds=  5 }  /* エレキブースター */,
        { .item_no=323, .need_step= 2500, .odds=  5 }  /* マグマブースター */,
        { .item_no= 80, .need_step= 2050, .odds= 10 }  /* たいようのいし */,
        { .item_no= 81, .need_step= 2000, .odds= 10 }  /* つきのいし */,
        { .item_no=110, .need_step= 1500, .odds= 10 }  /* まんまるいし */,
        { .item_no= 91, .need_step= 1000, .odds= 50 }  /* ほしのかけら */,
        { .item_no= 33, .need_step=  700, .odds= 50 }  /* モーモーミルク */,
        { .item_no= 32, .need_step=  600, .odds= 60 }  /* ミックスオレ */,
        { .item_no= 31, .need_step=  500, .odds= 70 }  /* サイコソーダ */,
        { .item_no= 30, .need_step=    0, .odds=100 }  /* おいしいみず */
    }
  }
};
