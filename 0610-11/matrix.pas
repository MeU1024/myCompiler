program main;
var
    a,b,c,d,i,j,k,tmp,m,n:longint;
    x,y,f:array[1..10000] of longint;

begin

read(a);
read(b);

for i:=1 to a do begin
    for j:=1 to b do begin
        tmp := (i - 1) * b + j;
        read(x[tmp]);
    end;
end;

read(c);
read(d);

for i:=1 to c do begin
    for j:=1 to d do begin
        tmp := (i - 1) * d + j;
        read(y[tmp]);
    end;
end;

if b<>c then begin
    writeln('Incompatible Dimensions');
    end
else begin

    for i:=1 to a do begin
        for j:=1 to d do begin
            tmp := (i - 1) * d + j;
            f[tmp] := 0;
            for k:=1 to b do begin
                m := (i - 1 ) * b + k;
                n := (k - 1 ) * d + j;
                f[tmp]:=f[tmp]+x[m]*y[n];
            end;
            if tmp mod d = 0 then begin
                writeln(f[tmp]);
            end
            else begin        
                write(f[tmp]);
            end;
        end;
    end;

end;
end. 

//read改名input
//声明前后增加标识符
