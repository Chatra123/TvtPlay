
## TvtPlay_mod

TVTest pluginのTvtPlayの改変版


-------------------------------------------------------------------------
### 変更点

* ”次のチャプター”　→　”次の本編開始チャプター”  
　”前のチャプター”　→　”前の本編開始チャプター”  
　に置き換え

　c-1000dix-1100dox-2000dix-2100dox-3000dix-0eox-c  
　通常なら dix, dox を順に移動するところを dox 間のみ移動します。  
　ＣＭ開始チャプターへの移動はできません。
  
  
* ”SeekA”　→　”＋１０秒シーク、スキップチャプター直前のみスキップ”  
　に置き換え  

　通常は＋１０秒シーク  
　スキップチャプター直前の３０秒間はスキップ  
　スキップ間が短く、スキップ直後３０秒間と直前３０秒間が重なると＋１０秒シーク  
　TvtPlay.iniのSeekAの設定は無視されます。  

![SeekA動作](./TvtPlay_mod_SeekA.png)


* シークバーの表示変更

![SeekBar表示](./TvtPlay_mod_SeekBar.png)


* ”常に前面に表示”　→　”再生中のみ前面に表示”  
　に置き換え  


* コマンドラインに.ts .tslist等があれば再生するようにした。  
  従来よりもプラグイン、ドライバを自動で有効にする範囲を拡大  
  

* ドライバ切り替え時に再生速度を１．０倍速に戻す。



-------------------------------------------------------------------------
### fork  

xtne6f/TvtPlay　work-plus branch  
<https://github.com/xtne6f/TvtPlay>  

      
### test environment  
  
TVTest 0.9.0dev  
DBCTRADO/TVTest  develop branch  
<https://github.com/DBCTRADO/TVTest>  
TVTest 0.8.1では動きません。


   