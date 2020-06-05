
// RELEASE, SLOW DUMPING STILL EXISTS (ONLY ON SERIAL), NO MORE RAM TRACKING,
// APROPRIATE SERIAL PRINTING IS STILL THERE BUT LINES ARE COMMENTED
#include "Adafruit_VL53L0X.h"
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
#include <math.h>


unsigned long previousMillis = 0;
int interval = 250; // laikas tarp matavimu (ms)
double intervalS; // laikas tarp matavimu (s)
byte mode = 0; // matavimo rezimas

// kitimas atstumo milimetrais:
int a1;
int a2;
int a3;
int a4;
// poslinkiai
double s1 = 0;
double s2 = 0;
// Pagrindiniai dydžiai:
double s = 0; // poslinkis
double v = 0; // greitis
double a = 0; // pagreitis
// average su apsauga:
int k = 0; // kintamasis
double sAv = 0; // vidutinė poslinkio reikšmė
double vAv = 0; // vidutinė greičio reikšmė
double aAv = 0; // vidutinė pagreičio reikšmė ABS


Adafruit_VL53L0X lox = Adafruit_VL53L0X();
void setup()
{
    intervalS = interval * 0.001;
    // SerialBootUp();

    lcd.begin(16, 2);

    if (!lox.begin()) // tikrinama sensoriaus stadija, jei neaptiktas, nieko nedarome iki restarto
    {
        lcd.print("Sensoriaus");
        lcd.setCursor(0, 1);
        lcd.print("klaida");
        while (1)
            ;
    }
    lcd.print("Matavimai 2020m.");
    delay(1000);
}

byte ReadButton()
{
    int x = analogRead(0);
    if (x < 60)
        return 1;
    if (x < 200)
        return 2;
    if (x < 400)
        return 3;
    if (x < 600)
        return 4;
    if (x < 800)
        return 5;

    return 0;
}

void loop()
{

    unsigned long currentMillis = millis();
    if (ReadButton() == 1) // vidurkio rezimas
    {
        sAv = 0;
        vAv = 0;
        aAv = 0;
        k = 0;
        mode = 1;
    }
    else if (ReadButton() == 2) // momentinis rezimas
    {
        mode = 0;
    }

    if (currentMillis - previousMillis >= interval)
    {
        Dump();
        previousMillis = currentMillis;
        Measure(); // matuoja atstumus pradinius
        if (a1) // ar jau nuskaite visus 4
        {
            Calculate(); // skaičiuoja pagrindinius dydzius ty. s,v
        }
    }
}

void PrintCurrentDataScreen()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("s:");
    lcd.setCursor(2, 0);
    lcd.print(s);
    lcd.setCursor(7, 0);
    lcd.print("v:");
    lcd.setCursor(9, 0);
    lcd.print(v);
    lcd.setCursor(0, 1);
    lcd.print("a:");
    lcd.setCursor(2, 1);
    lcd.print(a);
    lcd.setCursor(6, 1);
    lcd.print("  *moment.");
    lcd.setCursor(13, 0);
    lcd.print("   ");
}

void PrintCurrentDataSerial()
{
    if (Serial)
    {
        Serial.print("\ns: ");
        Serial.println(s);
        Serial.print("v: ");
        Serial.println(v);
        Serial.print("a: ");
        Serial.println(a);
    }
}

void PrintAverageDataSerial()
{
    if (Serial)
    {
        Serial.print("\nVIDUTINES REIKSMES: ");
        Serial.print("\nk: ");
        Serial.println(k);
        Serial.print("s: ");
        Serial.println(sAv);
        Serial.print("v: ");
        Serial.println(vAv);
        Serial.print("a: ");
        Serial.println(aAv);
    }
}

void PrintAverageDataScreen()

{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("s:");
    lcd.setCursor(2, 0);
    lcd.print(sAv);
    lcd.setCursor(7, 0);
    lcd.print("v:");
    lcd.setCursor(9, 0);
    lcd.print(vAv);
    lcd.setCursor(0, 1);
    lcd.print("a:");
    lcd.setCursor(2, 1);
    lcd.print(aAv);
    lcd.setCursor(6, 1);
    lcd.print("  *vid.  ");
    lcd.setCursor(13, 0);
    lcd.print("   ");
}


void PrintLoadingScreen()
{
    lcd.setCursor(0, 0);
    lcd.clear();
    lcd.print("Skaiciuojama.....");
    lcd.setCursor(0, 1);
    lcd.print(k);
    lcd.print("/25");
}

void CalcAvg()
{
    // average reikšmėms:
    if (k < 25) // per mazai duomenu
    {
        k++;
        sAv = sAv + s;
        vAv = vAv + v;
        aAv = aAv + abs(a);
        PrintLoadingScreen();
    }
    else if (k == 25) // apdorojami duomenys
    {
        sAv = sAv / k;
        vAv = vAv / k;
        aAv = aAv / k;
        k++;
    }
    if (k >= 25) // spausdinami duomenys
    {
        // PrintAverageDataSerial();
        PrintAverageDataScreen();
    }
}

void Calculate()
{
    s1 = abs(a1 - a2) * 0.001; // poslinkis1 m
    s2 = abs(a2 - a3) * 0.001;
    s = (s1 + s2) / 2; // poslinkis m
    if (s <= 0.01)
    {
        v = 0;
        s = 0;
        a = 0;
    }
    else
    {
        v = (s / intervalS); // greitis m/s
        // greiciai pagreiciui:
        double v1 = (s1 / intervalS); // greitis1
        double v2 = (s2 / intervalS); // greitis 2
        // pagreitis m/s^2:
        a = ((v2 - v1) / (intervalS * 2));
        if (mode == 1)
            CalcAvg();
    }
    if (mode == 0)
    {
        PrintCurrentDataScreen();
        // PrintCurrentDataSerial();
    }
}
void Measure()
{
    VL53L0X_RangingMeasurementData_t measure;
    lox.rangingTest(&measure, false); // pass in 'true' to get debug data printout!
    if (measure.RangeStatus != 4) // phase failures have incorrect data
    {

        a1 = a2; // perkeliamas praeitas matavimas i senesni
        a2 = a3;
        a3 = a4;
        a4 = measure.RangeMilliMeter; // pirmas ismatavimas atstumo
    }
}

void SerialBootUp()
{
    Serial.begin(115200);

    // wait until serial port opens for native USB devices
    while (!Serial)
    {
        delay(1);
    }

    Serial.println("Matavimai 2020m.");
    delay(1000);
}


void Dump()
{
    if (Serial)
    {
        Serial.println("==========================================================================="
                       "=============");
        Serial.print("Button ");
        Serial.println(ReadButton());
        Serial.print("Millis ");
        Serial.println(millis());
        Serial.print("unsigned long previousMillis ");
        Serial.println(previousMillis);
        Serial.print("int interval ");
        Serial.println(interval);
        Serial.print("double intervalS ");
        Serial.println(intervalS);
        Serial.print("byte mode ");
        Serial.println(mode);
        Serial.print("int a1 ");
        Serial.println(a1);
        Serial.print("int a2 ");
        Serial.println(a2);
        Serial.print("int a3 ");
        Serial.println(a3);
        Serial.print("int a4 ");
        Serial.println(a4);
        Serial.print("double s1 ");
        Serial.println(s1);
        Serial.print("double s2 ");
        Serial.println(s2);
        Serial.print("double s ");
        Serial.println(s);
        Serial.print("double v ");
        Serial.println(v);
        Serial.print("double a ");
        Serial.println(a);
        Serial.print("int k ");
        Serial.println(k);
        Serial.print("double sAv ");
        Serial.println(sAv);
        Serial.print("double vAv ");
        Serial.println(vAv);
        Serial.print("double aAv ");
        Serial.println(aAv);
        Serial.print("freeMemory()=");
        Serial.println();
    }
}
