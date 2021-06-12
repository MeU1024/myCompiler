program main;
var
        one,n,s:integer;
        a:array[0..2000000]of integer;

procedure qsort(l,r:longint);
var
  i,j,m,t:longint;
begin
  one :=1;
  i:=l;
  j:=r;
  m:=a[(l+r) div 2];
  while i<=j do begin
    while a[i]<m do i:=i+one;
    while a[j]>m do j:=j-one;
    if i<=j then
    begin
      t:=a[i];
      a[i]:=a[j];
      a[j]:=t;
      i:=i+one;
      j:=j-one;
    end;
  end;
  if l<j then qsort(l,j);
  if i<r then qsort(i,r);
end;

begin
    read(n);
    for s:=1 to n do begin
    	read(a[s]);
    end;
    qsort(1,n);
    for s:=1 to n do begin
        writeln(a[s]);
    end;
end.

