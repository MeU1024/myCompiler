program main;
var
    a,b,c,i,j,k,tmp,m,n:integer;
    x,y,f:array[1..100] of integer;

begin

read(a);
read(b);
read(c);

for i:=1 to a do begin
    for j:=1 to b do begin
        tmp := (i - 1) * b + j;
        read(x[tmp]);
    end;
end;

for i:=1 to b do begin
    for j:=1 to c do begin
        tmp := (i - 1) * c + j;
        read(y[tmp]);
    end;
end;

//writeln(a,' ',c);

for i:=1 to a do begin
    for j:=1 to c do begin
        tmp := (i - 1) * c + j;
        f[tmp] := 0;
        for k:=1 to b do begin
            m := (i - 1 ) * b + k;
            n := (k - 1 ) * c + j;
            f[tmp]:=f[tmp]+x[m]*y[n];
        end;        
    write(f[tmp],' ');
    end;
end;

end. 