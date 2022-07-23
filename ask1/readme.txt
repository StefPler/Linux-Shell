// Stefanos Pleros
// A.M: 3593

To shell ilopoieite se ena loop to opoio ekteleite ews wtou na patisei o xristis exit.
To prwto pragma pou ginete einai na tupwsw to prompt tou shell to opoio pernei to input
tou xristi. Se auth thn fasi diaxeizome kai to TAB opou kanei autocomplete file/directorynames. Afou to pliktrologisei tote uparxoun sinthikes diakladwshs pou xwrisonte se:

1. cd
2. exit
3. exec

to cd tha alaksei to directory tou xristi.
to exit tha stamatisei to execution tou shell
to exec tha ektelesei mia apo tis upolipes entoles tou shell.

sto exec block kwdika ginete diaxirisi twn wildcards kai pernaw ola ta wrismata
se mia char ** metabliti. An uparxei redirection tote prin to exec tha ginoun
oi kataliles diergasies wste na ginei anakateuthinsi eisodwn/eksodwn. An uparxei pipe
tote xwrizontes oi entoles kai ektelounte kanontas feed h mia thn alli, anoigontas prwta
ena pipe wste na boresoun na epikinonisoun. (upostirizei mono 1 pipe). An uparxei daemon 
operator tote den kanw wait sthn patriki diergasia. Gia na kanw
exec prwta kanw fork, dimiourgontas etsi mia diergasia paidi h opoia tha ektelesi
thn entoli tou xrisi. Meta to peras tou exec gurname ston pateras apo opou ola ta 
parapanw epanalambanontai. Pipes work only with space.

Perisoteres leptomeries: diaxeirizome malloc kai realloc (some checked for successful memory allocation) 
gia thn elaxisti dunati dunamiki desmeusi mnimis kai kanw free  metablites otan bgainoun ektos tou scope ths xrisis tous.
Xrisimopoiw merikes dikes mou boithitikes sunartiseis gia na glitwsw epanalipsi kwdika.
