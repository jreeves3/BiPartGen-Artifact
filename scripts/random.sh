#!/bin/sh

dir=`dirname $0`
mkdir -p $dir"/random130"

edgeCount=130

for encoding in "direct" "sinz" "linear" "mixed"
do
    mkdir -p $dir"/random130/"$encoding

    for n in {11..20}
    do
      out="random130/"$encoding"/A"$n
      echo $out
      ./../bipartgen -g random -n $n -f $out -e $encoding -E $edgeCount -v

      out="random130/"$encoding"/B"$n
      echo $out
      ./../bipartgen -g random -n $n -f $out -e $encoding -E $edgeCount -M -L -v

    done
done

