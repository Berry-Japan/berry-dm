#!/usr/bin/env bash 
#
echo -ne  "太字・下線・青色"
printf ' \e[1;4;34m sample \e[0m\n'


echo -ne "背景色を入れる "
printf "\e[106mここの背景は青い\e[0m\n"



echo -ne "文字色を反転"
printf "\e[7m 反転 \e[0m\n"


echo -ne " 点滅 " #ターミナルによっては反応しない
printf "\e[5m 文字を点滅 \e[0m\n"


echo -ne " 下線 "
printf "\e[4m 下線を引く \e[0m\n"


echo -ne " 太文字 "
printf "this is \e[1mBOLD\e[0m text.\n"


echo -ne " 太字下線を重ねがけ "

printf "this is \e[1;4m重ねがけ\e[0m text.\n"


echo -ne " 背景・文字色を重ねがけ "
printf "this is \e[106;97mBOLD\e[0m text.\n"

echo -ne " 256色から自由に選ぶ "
printf "this is \e[48;5;206;97;38;5;227mfull\e[0m \e[48;5;82;97;38;5;57mcolor\e[0m text.\n"


echo -ne '256色カラーサンプル(背景'
for c in {0..255}; do printf "\033[48;5;${c}m \033[m"; if [ $(($c%16)) == 0 ] ;then echo "";fi done; echo

