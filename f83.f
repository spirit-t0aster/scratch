variable sp0
variable 'tib
variable blk
variable span

CODE off (S addr -- ) BX POP FALSE # 0 [DX] MOV NEXT END-CODE


: begin	?<mark ; immediate
: then	?>resolve ; immediate
: do	compile (do) ?>mark ; immediate
: ?do	compile (?do) ?>mark ; immediate


: ] (s -- ) / the compile loop
state on
begin
	?stack defined dup
	if 
		0> 
		if 
			execute
		else
			,
		then
	else
		drop number double?
		if
			[compile] dliteral
		else
			drop [compile] literal
		then
	then
	true done?
until ;
: [ (s -- ) state off ; immediate / stop compiling and interpret
: : !csp current @ context ! create hide ] ;uses nest ,
: ; ?csp compile unnest reveal [compile] [ ; immediate
: [compile] (S -- ) ' , ; immediate




: expect dup span ! swap 0 ( len adr 0 )
begin 2 pick over - ( len adr #so-far #left )
while key dup bl <
	if	dup 2* cc @ + perform
	else	dup 127 = if del-in else char then
	then	repeat 2drop drop ;

: query tib 80 expect span @ #tib ! blk off >in off ;
: run state @ if ] state @ not if interpret then else interpret then ;
: quit sp0 @ 'tib ! blk off [compile] [
begin
	rp0 @ rp!
	status query run
	state @ not 
	if
		." ok"
	then
again ; 

defer boot
: warm (s -- )	true abort" warm start" ;
: cold (S -- )	boot quit ;
