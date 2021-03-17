#!/bin/sh

dir=`dirname $0`
mkdir -p $dir"/randomPGBDD130Ord"

edgeCount=130

for encoding in "sinz" "split"
do
    mkdir -p $dir"/randomPGBDD130Ord/"$encoding

    for n in {11..20}
    do
      out="randomPGBDD130Ord/"$encoding"/A"$n
      echo $out
      ./../bipartgen -g random -n $n -f $out -e $encoding -E $edgeCount -v -o

    done
done

mkdir -p $dir"/randomPGBDD130Sched/"

for n in {11..20}
  do
    out="randomPGBDD130Sched/A"$n
    echo $out
    ./../bipartgen -g random -n $n -f $out -e sinz -E $edgeCount -v -p
done
