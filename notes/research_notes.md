# IMS Data

## 1 Počáteční otázka

```latex
\subsection{Počáteční otázka}

Hlavní otázkou, kterou se studie zabývá, je \emph{Kde se v řetězci setí ozimé pšenice na průměrně velké farmě v ČR nacházejí kritická úzká místa a jaké změny v konfiguraci linky je nutné provést pro minimalizaci prostojů a bezpečné dodržení agrotechnického termínu?}.

V této simulační studii bude zodpovězeno také  několik dalších související otázek jako např. \emph{Který kritický technologický krok nebo zdroj (stroj, pracovník, \dots) vykazuje v simulovaném modelu nejnižší průchodnost a nejvyšší míru využití, čímž se stává hlavním úzkým hrdlem výrobní linky?} nebo \emph{Jak velký dopad by měla změna kapacity secího stroje či nasazení dalšího secího stroje nebo prodloužení směny pracovníků?}. V neposlední řadě studie analyzuje, jaký je pravděpodobnostní dopad vlivu stochastických událostí (např. výpadek secího stroje, náhlá změna počasí) na včasnost dokončení setí a jak se toto riziko promítne do ekonomické efektivity provozu a následného výdělku.
```

## 1 Fakta

### 1.1 Struktura českého zemědělství a význam ozimé pšenice

- Průměrná farma v ČŘ: 120 ha
- Průměrné pole v ČR: 15 ha
- Podíl ozimy na celkové zemědělské ploše: 28,9 %, cca 700-800 tis. ha
- Sklizeň zrna: cca 4,8 mil. tun (cca 6 t/ha)

### 1.2 Optimální termín setí ozimé pšenice

- v nižších polohách konec září až začátek října
- optimální okno setí: cca 20. 9. - 10. 10. (21 dní)

### 1.3 Vliv zpoždění setí na výnos - Pokles výnosu při pozdním setí ozimé pšenice

- pokles cca o 1 % na každý den zpoždění setí za optimální termín (jinde se uvádí 10-20 % za 14 dní ==> pokles 0,7-1,4 %)

### 1.4 Počasí a "pracovní dny" pro polní práce

- během dvou měsíců (září-říjen, ~60 dní) je pravděpodobnost "pracovního dne" někde kolem 0,35-0,45 (tato statistika se odvozuje podle vlhkosti apod.)

### 1.5 Výkonnost strojů a výpočet kapacity

- standardní zemědělská mechanizace používá definici **efektivní plošné výkonnosti** $[ha/h]$ (Effective Field Capacity, EFC), kterou lze aproximovat vztahem:
  $$
  \text{EFC} = \frac{W[\text{m}] \times S[\text{km/h}] \times \eta}{10},
  $$
  kde $W$ pracovní záběr, $S$ je pracovní rychlost a $\eta$ je "field efficiency" (typicky 0,6-0,85) (podle ztrát na otáčení, přejezdy, seřizování apod.).

- existuje také **teoretická plošná výkonnost** $[ha/h]$, kterou lze získat jako:
  $$
  \text{TFC} = \frac{\text{EFC}}{\eta} = \frac{W[\text{m}] \times S[\text{km/h}]}{10}
  $$

> Při výběru konkrétních strojů jsem zvolil takový stroj, který dle mého průzkumu internetových prodejců zemědělské techniky má průměrnou cenu a průměrnou teoretickou plošnou výkonnost $TFC$.
> Konkrétní hodnoty pracovní rychlosti $S$ a "field efficiency" $\eta$ jsou dle mého názoru spíše hypotézami než fakty, ale pro přehlednost, to uvedu v tomto přehledu strojů

- **Podmítací stroj:** ROL EX Gruber APG 3.0
  - Cena: cca 200 000 Kč
  - Pracovní záběr $W$: 3 m
  - Pracovní rychlost $S$: 8 km/h (6-12 km/h)
  - Field efficiency $\eta$: 0,65 (těžší záběr, častější zvedání, otáčení, úpravy hloubky)

- **Pluh:** AGROPA AGN21-440
  - Cena: cca 300 000 Kč
  - Pracovní záběr $W$: 1,8 m
  - Pracovní rychlost $S$: 6 km/h (5-7 km/h)
  - Field efficiency $\eta$: 0,75 (relativně hodně souvratí a přejezdů, ale práce je plynulá)

- **Kombinátor:** Campbell ROL EX WFC 3.0
  - Cena: cca 150 000 Kč
  - Pracovní záběr $W$: 3 m
  - Pracovní rychlost $S$: 8 km/h (4-12 km/h)
  - Field efficiency $\eta$: 0,8 (lehčí práce, méně nastavování, souvratě rychlé)

- **Kombinovaný mělký kypřič:** Finisher 400
  - Cena: cca 1,75 mil. Kč
  - Pracovní záběr $W$: 4,15 m
  - Pracovní rychlost $S$: 10 km/h (8-14 km/h)
  - Field efficiency $\eta$: 0,75 (lepší než orba, horší než čisté kombinování (válení))

- **Rozmetadlo hnoje:** Cynkomet N-221/3-3 CS LINE 6
  - Cena: cca 700 000 Kč
  - Kapacita: 6 tun hnoje
  - Pracovní záběr $W$: 7 m (6-8 m)
  - Pracovní rychlost $S$: 10 km/h (6-12 km/h)
  - Field efficiency $\eta$: 0,6 (relativně velké ztráty času na nakládku, přejezd k hromadě, rozjíždění, souvratě)

- **Secí stroj:** Unia Polonez 550/3D
  - Cena: cca 500 000 Kč
  - Kapacita: 550 litrů zrna
  - Pracovní záběr $W$: 3 m
  - Pracovní rychlost $S$: 7 km/h (5-9 km/h)
  - Field efficiency $\eta$: 0,7 (více přejezdů kvůli doplňování osiva, ladění dávky/ hloubky, delší souvratě)

- **Kultivační válec:** Cambridge ROL EX WPH 4.5
  - Cena: cca 300 000 Kč
  - Pracovní záběr $W$: 4,5 m
  - Pracovní rychlost $S$: 6 km/h (4,5-7,5 km/h)
  - Field efficiency $\eta$: 0,85 (jednoduchý stroj s minimálním nastavováním, otáčení a souvratě jsou rychlé)

### 1.6 Konvenční vs. redukovaná technologie zpracování půdy (časové nároky)

- Eurostatův indikátor "Agri-environmental indicator - tillage practices" uvádí, že **reduced tillage** může snížit potřebu pracovních hodin řádově o **30-40 %** oproti klasické orbě - v závislosti na regionu. 
- Ekonomické a agronomické studie (např. Calcante 2019, Woźniak 2020, Moldovan aj.) potvrzují, že **minimum-till a no-till dramaticky snižují energetické i časové nároky na ha** a často i celkové náklady (uvedeny rozdíly 16-20 % nákladů nákladů a výrazně nižší spotřeba práce).

### 1.7 Poruchovost a spolehlivost traktorů - nemá cenu řešit v našem případě jako takovou, ale pro úplnost uvedu

- spolehlivost traktorů se typicky modeluje pomocí exponenciálního nebo Weibullova rozložení
- MTTF = mean time to failure
- MTBF = mean time between failures
- u trakotrů apod. se to pohybuje často v řádu tisíců motohodin (spolehlivost někde mezi cca 8200-11100 hodin)

## 2. Hypotézy (co bych navrhl zjednodušit nebo jsem už u výše házel jen cca)

### 2.1 Hypotézy o velikosti farmy a počtu hektarů pro ozim

- mějme rozlohou mírně nadprůměrnou farmu s rozlohou třeba 180 ha, kde stejně jako v celorepublikovém průměru je ozim pěstována na cca 30 % zemědělské půdy -> tedy zabýra krásných 60 ha
- jelikož má 60 ha polí pro ozim -> $60 / 15 = 4$ -> plocha pro ozim je rozdělena do $4\times15$ ha průměrně velkých polí

### 2.2 Hypotézy o lince

- **Sekvence operací:**
  - Zjednodušená sekvence operací pro konvenční i min-till variantu:
    - **konvenční:** podmítka --> orba --> předseťová příprava --> hnojení --> setí --> zaválení
    - **min-till:** kombinované mělké kpření --> hnojení --> setí --> zaválení
- **Časová úspora min-till**
  - Zdroje se v tomhle údaji dost liší -> uvádí úsporu práce relativně v rozmězí 30-75 % (méně práce)
  - V modelu to můžeme případně řešit těmahle dvěma způsoby, co mě zrovna napadjí:
    - a) jednodušše použít konstantu $C_{min-till}$, která by třeba řekla "Pro min-till bude čas na ha násoben 0,7." (maximální teoretická úspora je 50 % času na hektar).
    - b) simulovat "min-till" variantu podobně, jak ji mám v konceptuálním modelu
    - c) třeba tě napadne něco jiného
    - d) třeba min-till ani simulovat nebudeme
- **Efektivní plošná výkonnost:**

  $$
  \text{EFC}_{\text{podmítací stroj}} = \frac{3 \cdot 8 \cdot 0{,}65}{10} = 1{,}56~\text{ha/h}
  $$

  $$
  \text{EFC}_{\text{pluh}} = \frac{1{,}8 \cdot 6 \cdot 0{,}75}{10} = 0{,}81~\text{ha/h}
  $$

  $$
  \text{EFC}_{\text{kombinátor}} = \frac{3 \cdot 8 \cdot 0{,}8}{10} = 1{,}92~\text{ha/h}
  $$

  $$
  \text{EFC}_{\text{kombinovaný mělký kypřič}} = \frac{4{,}15 \cdot 10 \cdot 0{,}75}{10} = 3{,}11~\text{ha/h}
  $$

  $$
  \text{EFC}_{\text{rozmetadlo hnoje}} = \frac{7 \cdot 10 \cdot 0{,}6}{10} = 4{,}20~\text{ha/h}
  $$

  $$
  \text{EFC}_{\text{secí stroj}} = \frac{3 \cdot 7 \cdot 0{,}7}{10} = 1{,}47~\text{ha/h}
  $$

  $$
  \text{EFC}_{\text{kultivační válec}} = \frac{4{,}5 \cdot 6 \cdot 0{,}85}{10} = 2{,}30~\text{ha/h}
  $$

- **Můj návrh časové abstrakce:**
  - máme vlastně 3 hlavní časová okna:
    - **před setím:** zpracování půdy a hnojení
      - **1\. podmítka** jednoho 15 ha políčka trvá $\beta$-distribucí 9,5 hodin.
      - **2\. orba** jednoho 15 ha políčka trvá $\beta$-distribucí 15,5 hodin.
      - **3\. předseťová úprava** jednoho 15 ha políčka trvá $\beta$-distribucí 8 hodin.
      - **4\. hnojení** jednoho 15 ha políčka trvá $\beta$-distribucí 3,5 hodiny.
      - Alternativně lze **1.-3.** nahradit pomocí **kombinovaného mělkého kypření (min-till)**, které pro jedno 15 ha políčko trvá $\beta$-distribucí 5 hodin.
    - **setí** (tj. to hlavní)
      - **setí** jednoho 15 ha políčka trvá $\beta$-distribucí 10 hodin.
    - **po setí:** zaválení půdy
      - **zaválení** jednoho 15 ha políčka trvá $\beta$-distribucí 6,5 hodiny.

### 2.3 Hypotézy o traktorech a strojích

- **Konkrétní stroje:**
  - uvažujme třeba 2 traktory
  - uvažujme 1 stroj od každého
- **Konkrétní rychlosti a efektivita pole**
  - viz výše
- **(NE)dostupnost traktorů:**
  - Základní rozhodnotí je, jestli traktor **vyjede** nebo **NEvyjede**
    - Traktor **NEvyjede**, pokud den "není pracovní" - vůbec neuvažujeme, že by nějaký traktor mohl vyjet a čekáme na další den.
    - Traktor **může vyjet**, pokud je "pracovní den"
      - Traktor **vyjede**, pokud je na směně nějaký pracovník A SOUČASNĚ je nějaký traktor na farmě ("v depu").
      - Pokud není zrovna traktor na farmě nebo pracovník na směně, tak se nic nestane.
    - Pokud traktor s pracovníkem vyjede je zde *určitá šance*, že nevyjel kvůli pěstování ozimé pšenice
      - *určitá šance* je tvořena
        - 1\. traktor se porouchá nebo musí absolvovat rutinní kontrolu --> to může zabrat třeba $\beta$-distribucí 4 hodiny.
        - 2\. traktor vyjel pěstovat jinou plodinu, což mu zabere celou délku směny (nebo zbytek rozdělané směny) jeho pracovníka.

### 2.5 Hypotézy o dělnících

- **Počet dělníků:**
  - tady jde čistě o hypotetické číslo: třeba pro začátek stejný jako počet trsaktorů
  - pracovník má univerzální roli
- **Délka směny:**
  - Při sezónních špičkách je typicky normální směna 10 h a prodloužená směna 12 h
  - Každopádně je to opět jen hypotéza (neexsituje žádný meřič standardu)
- **Pauzy a střídání směn:**
  - pracovník po dokončení své směny zbývající čas dne (tedy $24\ hod. - T_{směna}$) odpočívá a další den opět vesele nastoupí do práce (při implementaci bude pak nutné dodržet synchronizaci příchodu "(NE)pracovního" a tento 24hodinnové cyklus pracovníků)
  - Můžeme třeba zkusit simulovat 2směnný provoz (to nechám na tobě)
- V konceptuálním modelu modeluji, že pracovník má v sobě paměť kolik času do konce směny mu zbývá.
  - Práce na poli může tedy skončit těmito dvěma způsoby:
    - Pracovník dokončil svoje políčko před uplynutím jeho pracovní doby --> vrací na farmu opět pracovat a odevzdává traktor (a jakoby si ho opět znovu zabere ale možné, že to nebude kvůli práci na ozimé pšenici)
    - Pracovníkovi skončila pracovní doba, ale svoje políčko ještě nedokončil --> vrací se na farmu, odevzdává traktor a odchází na odpočinek (políčka mají memory, kolik zbývá do konce rozpracované činnosti - např. orby)
- Pokud pracovník s traktorem vyjel kvůli ozimé pšenici, prioritně se vrhne na práci, která je v "pipelině práce" jako v první (tedy pokud bychom měli políčko, které čeká na orbu a políčko, které čeká už na hnojení, pracovník se prioritně vrhne na orání, protože orba předchází hnojení)

### 2.6 Hypotézy o počasí

- **"Pracovní"/"NEpracovní" dny**
  - pro každý den v období září-říjen ($\approx$ 60 dní) by bylo náhodně náhodně rozhodnuto, jestli to bude "pracovní" nebo "nepracovní"
  - ve faktech jsem našel, že to je pravděpodobnost těch $0,35-0,45$, že den bude pracovní -> proto bych navrhoval dát $C_{workable}=0,40$
  - v realitě je $C_{workable}$ závislá na složení půdy (jak kvalitně dokáže přijímat vodu a následně ji držet - jíl, černozem, apod.) a konkrétní deštivosti daného podzimu
    - to bych právě ignoroval a fláknul tam konstantu (jak říkal, že si můžeme určitý okrajový fakta zjednodušit)
- Mohli bysme třeba dělat experiment pro příliš deštivý rok, kdy by pravděpodobnost, že den je pracovní mohla klesnout na $C_{workable}=0,25$ (kdybysme do dělali, dohledal bych konkrétní data na meteorologickém úřadu, abysme tam jen tak neplácli hodnotu a on nám to nezkritizoval)

### 2.7 Několik ekonomických hypoté

- Cena ozimi za tunu a hodinová cena provozu strojů by mohly být zvoleny jako konstanty, bez stochastiky a bez rizika trhu.
- Šlo by to celé redukovat na: $\text{Tržba} = \text{výnos} \cdot \text{cena za tunu} - \text{provozní náklady strojů}$
- Dále bychom mohli počítat s tím faktem, že jeden den zpoždění setí vyústí v 1% pokles zisku (příp. komplexněji 0,7-1,3% pokles rovnoměrně)

## 3 Hlavní omezení v sytsému

- Počasí
- Traktor/y
- Stroj/e
- Pracovníci