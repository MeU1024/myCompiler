program main;
var
        n,i:integer;
        a:array[0..2000000]of qword;
procedure kp(l,r:longint);
var
        i,j,mid:integer;
begin
        if l>=r then exit;
                i:=l;j:=r;mid:=a[(l+r) div 2];
        repeat
         while a[i]<mid do inc(i);
                while a[j]>mid do dec(j);
                        if i<=j then
                        begin
                                a[0]:=a[i];a[i]:=a[j];a[j]:=a[0];
                                inc(i);dec(j);
                        end;
        until i>j;
    kp(l,j);
    kp(i,r)
end;
begin
    readln(n);
    for i:=1 to n do
    read(a[i]);
    kp(1,n);
    for i:=1 to n do
    writeln(a[i]);

end.
