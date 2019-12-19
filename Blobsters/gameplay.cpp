  // 
// 
// 

#include "gameplay.h"
#include "sprites.h"



#define ZERO 1e-10
#define isBetween(A, B, C) ( ((A-B) > - ZERO) && ((A-C) < ZERO) )
#define SCREEN_WIDTH 320	//window height
#define SCREEN_HEIGHT 240	//window width

/* To do:
1) Handle fridge showing multiple pages
2) Make fridge "Sold out" thing not look shit (Mostly fixed, still leftover "max size" errors and only first one gets new ico 
3) Don't let player overfill inventory
4) Have shop change on timer, not reset 
5) Add shop stock to save
6) Add age to save
7) Fix button bounce
8) Add coins to summary screen
9) HiLo sound system is a bit shit
10) HiLo seems to give wrong answers

MG Ideas:
1) HiLow
2) Pong
3) Ball Maze (Gyro)
4) Treasure Hunt (Magnetic sensor???)
5) Fruit Catch
6) Coin Catch (Gyro) 
7) "Guitar Hero"
8) Jumpy Pet
9) Button Masher
10) Simon Says
*/

TFT_eSPI tft = TFT_eSPI();
foodItem foodListC[FOOD_QTY];
audio aPlayer;



void gamePlay::idleLoop(Inventory* curInventory)
{
	m_shouldContinue = true;
	int screensizex = 320;
	int screensizey = 240;
	int16_t xPosMod = 200;
	int16_t yPosMod = 180;
	uint16_t transparent = 0xFFFF;
	tft.setSwapBytes(true);

	for (int x = 0; x < 320; x = x + 16) {
		for (int y = 175; y < 260; y = y + 16) {
			tft.pushImage(x, y, 16, 16, floor_tile);
		}
	}
	m5.Lcd.drawPngFile(SD, "/bg/main_fancy.png", 0, 0);
	//
	while (true) {
		int rando = (rand() % 10) + 1;
		if (rando < 5) {
			if (xPosMod - 5 > 60) {
				xPosMod = xPosMod - 16; //Go backwards
			}
		}
		else if (rando >= 5) {
			if (xPosMod + 5 < (screensizex - 64)) {
				xPosMod = xPosMod + 16; //Go forward
			}
		}
		tft.pushImage(xPosMod, yPosMod, 64, 64, blue_front, transparent);
		delay(1000);
		refloor(0, 176, 320, 260, xPosMod, yPosMod, floor_tile, 64, 64);
		m5.update(); 
		aPlayer.wavloop();
		if (checkInterrupts == true) {
			aPlayer.forceStop();
			timerHandler(curInventory);
			checkInterrupts = false; 
		}
		if (btnAPress == true) {
			aPlayer.playSound(scButtonA);
			clearButtons();
			aPlayer.forceStop();
			showStats(curInventory);
			idleLoop(curInventory);
		}
		if (btnBPress == true) {
			aPlayer.playSound(scButtonB);
			clearButtons();
			aPlayer.forceStop();
			showMap(curInventory);
			idleLoop(curInventory);
		}
		if (btnCPress == true) {
			aPlayer.playSound(scButtonC);
			clearButtons();
			aPlayer.forceStop();
			showInventory(curInventory); 
			idleLoop(curInventory);
		}
	}
}

template <typename T>
bool IsInBounds(const T& value, const T& low, const T& high) {
	return !(value < low) && !(high < value);
}

void gamePlay::setUp() {
	tft.init();
	tft.setRotation(1);
	//Only run this once!
	strcpy(cChar.colour, "Blue");
	strcpy(cChar.name, "Thingy");
	cChar.fullness = 10;
	cChar.happiness = 10;
}

void gamePlay::genShopItems() {
	foodItem thisFoodItem;
	food curFoods;
	for (int i = 0; i < FOOD_QTY; i++) {
		thisFoodItem = curFoods.genFoods(1, 0);
		foodListC[i] = thisFoodItem;
	}
}

void gamePlay::showShop(Inventory* curInventory) {
	food curFoods;
	int curSlot = 0; 
	bool updateStock = false; 
	m5.Lcd.drawPngFile(SD, "/bg/Shop.png", 0, 0);
	tft.setTextSize(2);
	tft.setTextColor(TFT_BLACK, 0x92A7);
	int curX = 98;
	int curY = 28; 
	
	char priceString[4];
	for (int i = 0; i < FOOD_QTY; i++) {
		const char* filePath = foodListC[i].filepath;
		if (strcmp(filePath, "/food/OOS.png") == 0) {
			m5.Lcd.drawPngFile(SD, filePath, curX - 5, curY - 5, 40, 40);
		}
		else {
			m5.Lcd.drawPngFile(SD, filePath, curX, curY, 40, 40);
		}
		curX = curX + 52;
		if (i == 2) {
			curY = curY + 48; 
			curX = 98; 
		}
	}
	while (!btnCPress) {
		m5.update(); 
		aPlayer.wavloop();
		Serial.write("INIT LOOP DONE \n"); 
		tft.drawString(foodListC[curSlot].foodName, 80, 165);
		itoa(foodListC[curSlot].price, priceString, 10);
		tft.drawString(priceString, 150, 195);
		if (btnAPress == true) {
			aPlayer.playSound(scButtonA);
			clearButtons();
			tft.drawString("---", 150, 195);
			if (curSlot == FOOD_QTY - 1){
				curSlot = 0; 
			}
			else{
				curSlot = curSlot + 1;
			}
		}
		if (btnBPress == true) {
			clearButtons();
			aPlayer.playSound(scButtonB);
			if (foodListC[curSlot].price > 0) {
				int curX = 98;
				int curY = 28; 
				foodItem toAdd = foodListC[curSlot];
				toAdd.fdID = foodListC[curSlot].fdID; 
			//	curInventory->foodIList[curInventory->numOfFoods + 1] = toAdd; 
				DUMP(curInventory->numOfFoods);
				memcpy(&curInventory->foodIList[(curInventory->numOfFoods)], &toAdd, sizeof(toAdd));  
				curInventory->numOfFoods = curInventory->numOfFoods + 1;
				Serial.write("Bought food with ID : \n");
				DUMP(toAdd.fdID); 
				foodListC[curSlot] = curFoods.giveOOS();
				for (int i = 0; i < FOOD_QTY; i++) {
					const char* filePath = foodListC[i].filepath;
					if (strcmp(filePath, "/food/OOS.png") == 0) {
						m5.Lcd.drawPngFile(SD, filePath, curX - 5, curY - 5, 40, 40);
					}
					else {
						m5.Lcd.drawPngFile(SD, filePath, curX, curY, 40, 40);
					}
					curX = curX + 52;
					if (i == 2) {
						curY = curY + 48;
						curX = 98;
					}
				}
			}
			else {
				//You can't buy an out of stock item. Link in sound sys to play a uh-uh sound 
			}

		}
	}
	if (btnCPress == true) {
		aPlayer.playSound(scButtonC);
		aPlayer.forceStop(); 
		clearButtons();
		return;
	}
}

void gamePlay::showStats(Inventory* curInventory) {
	m5.Lcd.drawPngFile(SD, "/bg/summ.png", 0, 0);
	char str[2];
	int x = 82;
	int y = 115;
	for (int i = 0; i < cChar.fullness; i++) {
		tft.fillRect(x, y, 16, 12, TFT_RED);
		x = x + 16; 
	}
	x = 82;
	y = 80;
	for (int i = 0; i < cChar.happiness; i++) {
		tft.fillRect(x, y, 16, 12, TFT_RED);
		x = x + 16;
	}
	tft.setTextSize(6);
	tft.setTextColor(TFT_BLACK, TFT_WHITE);
	tft.drawString(cChar.name, 90, 25); 
	while (!btnCPress) {
		m5.update();
		aPlayer.wavloop();
		if (btnAPress == true) {
			aPlayer.playSound(scButtonA);
			clearButtons();
		}
		if (btnBPress == true) {
			aPlayer.playSound(scButtonB);
			clearButtons();
		}
	}
	if (btnCPress == true) {
		aPlayer.playSound(scButtonC);
		clearButtons();
		aPlayer.forceStop();
		return;
	}
}
 
void gamePlay::showMap(Inventory* curInventory) {
   int curPos = 0;
   int textX = 16;
   int textY = 192;
   int curWinnings = 0; 
   tft.setTextColor(TFT_BLACK);
   tft.setTextSize(4);
   m5.Lcd.drawPngFile(SD, "/bg/menu.png", 0, 0);
   tft.drawString(mapLocationNames[0], textX , textY);
	while (!btnCPress) {
		m5.update();
		aPlayer.wavloop();
		if (btnAPress == true) {
			aPlayer.playSound(scButtonA);
			clearButtons();
			delay(500);
			refloor(12, 192, 112, 224, textX, textY, map_box_tile, 160, 32);
			curPos = curPos + 1;
			if (curPos > 2) {
				curPos = 0;
			}
			tft.drawString(mapLocationNames[curPos], textX, textY);
		}
		if (btnBPress == true) {
			aPlayer.playSound(scButtonB);
			clearButtons();
			if (strcmp(mapLocationNames[curPos], "Shop") == 0){
				showShop(curInventory);
				aPlayer.forceStop();
				return;
			}
			else if (strcmp(mapLocationNames[curPos], "Play") == 0) {
				int earn = gameBoard(); 
				curWinnings = curWinnings + earn;
				curInventory->currentMoney = curInventory->currentMoney + curWinnings;
				aPlayer.forceStop();
				return;
			}
			else if (strcmp(mapLocationNames[curPos], "Meet") == 0) {
				//Do the meet
			}
		}
	}
	if (btnCPress == true) {
		aPlayer.playSound(scButtonC);
		clearButtons();
		delay(200); 
		aPlayer.forceStop();
		return;
	}
}

int gamePlay::gameBoard() {
	int curPos = 0;
	int curWinnings; 
	int textX = 70;
	int textY = 30;
	tft.setTextColor(TFT_BLACK, 0x4B8F);
	tft.setTextSize(2);
	m5.lcd.drawPngFile(SD, "/bg/GameBoard.png", 0, 0);
	//
	for (int i = 0; i < 8; i++) {
		if (i == curPos) {
			tft.setTextColor(TFT_BLACK, 0x4B8F);
		}
		else {
			tft.setTextColor(TFT_WHITE, 0x4B8F);
		}
		tft.drawString(gameNames[i], textX, textY);
		textY = textY + 20;
	}
	textY = 30; 
	while (!btnCPress) {
		aPlayer.wavloop();
		if (btnAPress == true) {
			aPlayer.playSound(scButtonA);
			clearButtons();
			delay(100);
			curPos = curPos + 1; 
			if (curPos > 7) {
				curPos = 0;
			}
			for (int i = 0; i < 8; i++) {
				if (i == curPos) {
					tft.setTextColor(TFT_BLACK, 0x4B8F);
				}
				else {
					tft.setTextColor(TFT_WHITE, 0x4B8F);
				}
				tft.drawString(gameNames[i], textX, textY);
				textY = textY + 20;
			}
			textY = 30;
		}
		if (btnBPress == true) {
			aPlayer.playSound(scButtonB);
			clearButtons();
			delay(500); 
			if (strcmp(gameNames[curPos], "HiLo") == 0) {
				aPlayer.forceStop(); 
				curWinnings = game_highlow();
				return curWinnings;
			}
			else if (strcmp(gameNames[curPos], "Pong") == 0) {
				aPlayer.forceStop();
				curWinnings = game_pong();
				return curWinnings;
			}
			else if (strcmp(gameNames[curPos], "Game3") == 0) {
				//DO THE GAME
			}

		}
	}
	if (btnCPress == true) {
		aPlayer.playSound(scButtonC);
		clearButtons();
		delay(500);
		aPlayer.forceStop();
		return curWinnings;
	}
}

void gamePlay::showInventory(Inventory* curInventory)
{
	//@TO DO: actually add error to shop when fridge full 
	int textX = 137;
	int textY = 41;
	int curPos = 0; 
	tft.setTextColor(TFT_BLACK);
	tft.setTextSize(2);
	m5.lcd.drawPngFile(SD, "/bg/inv_choice.png", 0, 0);
	//
	tft.drawString(invLocationNames[0], textX, textY);
	while (!btnCPress) {
		m5.update();
		aPlayer.wavloop();
		if (btnAPress == true) {
			aPlayer.playSound(scButtonA);
			clearButtons();
			refloor(137, 41, 238, 71, textX, textY, inv_box_tile, 160, 32);
			curPos = curPos + 1;
			if (curPos > 1) {
				curPos = 0;
			}
			tft.drawString(invLocationNames[curPos], textX, textY);
		}
		if (btnBPress == true) {
			aPlayer.playSound(scButtonB);
			clearButtons();
			if (strcmp(invLocationNames[curPos], "Food") == 0) {
				clearButtons();
				showFridge(curInventory);
			}
			else if (strcmp(invLocationNames[curPos], "Items") == 0) {
				clearButtons();
				//Show other
			}
		}
	}
	if (btnCPress == true) {
		aPlayer.playSound(scButtonC);
		clearButtons();
		aPlayer.forceStop(); 
		return;
	}
}

void gamePlay::showFridge(Inventory* curInventory) {
	//TO DO Handle page switching, replace "sold out" with eaten, reshuffle food when exiting fridge (or now?) 
	int itemSize = curInventory->numOfFoods;
	foodItem curFoodItem;
	food curFoods;
	int curX = 50;
	int curY = 30;
	int maxSize;
	char fillString[3];
	m5.Lcd.drawPngFile(SD, "/bg/Fridge.png", 0, 0);
	int fridgePage = 0;
	int foodSlot = 0;
	if (itemSize < 10) {
		maxSize = itemSize;
	}
	else {
		maxSize = 10;
	}
	tft.setTextSize(1);
	tft.drawString("0", 45, 205);
	tft.setTextSize(2);
	tft.setTextColor(TFT_BLACK, 0x4B8F);
	for (int i = 0; i < maxSize; i++) {
		curFoodItem = curInventory->foodIList[i];
		const char* filePath = curFoodItem.filepath;
		m5.lcd.drawPngFile(SD, filePath, curX, curY, 40, 40);
		curX = curX + 52;
		if (i == 4) {
			curY = curY + 48;
			curX = 50;
		}
	}
	while (!btnCPress) {
		aPlayer.wavloop();
		m5.update();
		tft.drawString(curInventory->foodIList[foodSlot].foodName, 80, 120);
		itoa(curInventory->foodIList[foodSlot].fill, fillString, 10);
		tft.drawString(fillString, 150, 155);
		/*Serial.write(curInventory->foodIList[foodSlot].foodName);*/
		if (btnAPress == true) {
			aPlayer.playSound(scButtonA);
			curFoodItem = curInventory->foodIList[foodSlot];
			clearButtons();
			tft.drawString("  ", 150, 155);			
			if (foodSlot == itemSize - 1) {
				foodSlot = 0;
			}
			else {
				foodSlot = foodSlot + 1;
			}
		}
		if (btnBPress == true) {
			if (curInventory->foodIList[foodSlot].price > 0) {
				aPlayer.playSound(scButtonB);
				clearButtons();
				int curX = 50;
				int curY = 30;
				curInventory->numOfFoods = curInventory->numOfFoods - 1;
				maxSize--; 
				curInventory->foodIList[foodSlot] = curFoods.giveEaten();
				cChar.fullness = cChar.fullness + curInventory->foodIList[foodSlot].fill;
				for (int i = 0; i < maxSize; i++) {
					DUMP(maxSize); 
					curFoodItem = curInventory->foodIList[i];
					const char* filePath = curFoodItem.filepath;
					if (strcmp(filePath, "/food/OOS.png") == 0) {
						m5.Lcd.drawPngFile(SD, filePath, curX - 5, curY - 5, 40, 40);

					}
					else {
						m5.Lcd.drawPngFile(SD, filePath, curX, curY, 40, 40);
						
					}
					curX = curX + 52;
					if (i == 4) {
						curY = curY + 48;
						curX = 50;
					}
				}
			}
			else {
				//can't eat what doesn't exist
			}
			if (btnCPress == true) {
				aPlayer.playSound(scButtonC);
				clearButtons();
				return;
			}
		}
	}
}

void gamePlay::clearButtons() {
	btnAPress = false;
	btnBPress = false;
	btnCPress = false; 
}

//Params: x/yMin = minimum x/y, x/yMax = max x/y Pos, tile = tile sprite, x/ypos mod = x/y of where sprite is, x/yMod = how big the thing to cover is 
void gamePlay::refloor(int xMin, int yMin, int xMax, int yMax, int xPosMod, int yPosMod, const short unsigned int* tile, int xMod, int yMod) {
	for (int x = xMin; x < xMax; x = x + 16) {
		for (int y = yMin; y < yMax; y = y + 16) {
			if (IsInBounds(x, xPosMod - xMod, xPosMod + xMod)) { // Does this X pos need updating
				tft.pushImage(x, y, 16, 16, tile);
			}
			if (IsInBounds(y, yPosMod - yMod, yPosMod + yMod)) { // Does this Y pos need updating 
				tft.pushImage(x, y, 16, 16, tile);
			}

		}
	}
}

void gamePlay::interruptTimer()
{
	//This will do for now, may want more complicated food thingy eventually, esp as we need to use this timer
	//For other times like aging and shop reset n stuff
	checkInterrupts = true; 
	hungerInterrupts++; 
	happinessInterrupts++;
	saveInterrupts++; 
}

void gamePlay::timerHandler(Inventory* curInventory) {
	//Each count of interrupt means 10 minutes has passed, so can change timers for gameplay feel based on that
	//TO DO: Implement death 
	//TO DO: Implement age in stats & save files
	if (hungerInterrupts > 6) {
		if (cChar.fullness > 1) {
			cChar.fullness--; 
		}
		else {
			cChar.isAlive = false;
		}
		hungerInterrupts = 0; 
	}
	if (happinessInterrupts > 12) {
		if (cChar.happiness > 1) {
			cChar.happiness--;
		}
		happinessInterrupts = 0;
	}
	if (saveInterrupts > 3) {
		saveGameData(curInventory);
		saveInterrupts = 0;
	}
	if (ageInterrupts > 143) {
		cChar.age++; 
		ageInterrupts = 0; 
	}
}

void gamePlay::interruptAbtn()
{
	btnAPress = true;
}

void gamePlay::interruptBbtn()
{
	btnBPress = true; 
}

void gamePlay::interruptCbtn()
{
	btnCPress = true; 
}

void gamePlay::saveGameData(Inventory* curInventory) {
	//TO DO: FIX SAVING FOOD ID 0 TO FILE ?!?!?!
	File sfile = SD.open("/save.txt", FILE_WRITE);
	if (SD.exists("/save.txt")) {   // If the file is available, write to it
		Serial.write("WROTE TO FILE \n"); 
		sfile.printf("Nam:%s*", cChar.name);
		sfile.printf("\n Col:%s*", cChar.colour);
		sfile.printf("\n Ful:%d*", cChar.fullness);
		sfile.printf("\n Hap:%d*", cChar.happiness);
		sfile.printf("\n Alv:%d*", cChar.isAlive);
		//Inventory 
		//sfile.printf("\n NoF:%d", curInventory->numOfFoods);
		DUMP(curInventory->numOfFoods);
		if (curInventory->numOfFoods > 0) {
			sfile.printf("\n Fli:( ");
		}
		for (int i = 0; i < curInventory->numOfFoods; i++) {
			if (curInventory->foodIList[i].fdID == 0) {
				Serial.write("FOOD ID IS 0 \n"); 
				DUMP(curInventory->foodIList[i].fdID);
				//Don't save it 
			}
			if (i == (curInventory->numOfFoods - 1)) {
				Serial.write("NOT BROKEN \n"); 
				DUMP(curInventory->foodIList[i].fdID);
				sfile.printf("%d )*", curInventory->foodIList[i].fdID);
			}
			else {
				sfile.printf("%d,", curInventory->foodIList[i].fdID);
				DUMP(curInventory->foodIList[i].fdID);
				Serial.write("NOT BROKEN \n");
			}
		}
		sfile.printf("\n Mon:%d*", curInventory->currentMoney);
		sfile.close(); 
	}
	else {
		Serial.write("CANT WRITE TO FILE \n"); 
	}
 }

void gamePlay::loadGameData(Inventory* curInventory) {
	String sFileString;
	char* resStr; 
	food curFoods; 
	int numFood = 0;
	File sfile = SD.open("/save.txt", FILE_READ);
	if (SD.exists("/save.txt")) {   // If the file is available, read from it
		sFileString = sfile.readString();
		DUMP(sFileString);
		Serial.write("/n"); 
	}
	else {
		Serial.write("No save found \n");
		return;
	}
	//Name
	resStr = findInFile("Nam:", sFileString);
	DUMP(resStr); 
	if (strlen(resStr) > 0) {
		Serial.printf("Char name is %s \n", resStr); 
		strcpy(cChar.name, resStr);
	}
	//Colour
	resStr = findInFile("Col:", sFileString);
	resStr = ""; 
	if (strlen(resStr) > 0) {
		Serial.printf("Char col is %s \n", resStr);
		strcpy(cChar.colour, resStr);
	}
	//Hunger
	resStr = findInFile("Ful:", sFileString);
	if (strlen(resStr) > 0) {
		Serial.printf("Char full is %s \n", resStr);
		cChar.fullness = atoi(resStr);
	}
	//Happy
	resStr = findInFile("Hap:", sFileString);
	if (strlen(resStr) > 0) {
		Serial.printf("Char hap is %s \n", resStr);
		cChar.happiness = atoi(resStr);
	}
	//Alive??
	resStr = findInFile("Alv:", sFileString);
	if (strlen(resStr) > 0) {
		Serial.printf("Char is alive?  %s \n", resStr);
		cChar.isAlive = atoi(resStr);
	}

	resStr = findInFile("Fli:", sFileString);
	char allNumbers[200];
	char delim[] = ",";
	int curSlot = 0; 
	if (strlen(resStr) > 0) {
			removeChar(resStr, 0);
			removeChar(resStr, (strlen(resStr)-1));
			/*Serial.write("REMOVED BRACKETS \n");*/
			DUMP(resStr); 
			strcpy(allNumbers, resStr);
			char* ptr = strtok(allNumbers, delim);
			/*Serial.write("Checking ptr \n");*/
			while (ptr != NULL) {
				/*Serial.write("Ptr not null \n");*/
				DUMP(ptr);
				if (ptr == NULL) {
					break;
				}
				int fdID = atoi(ptr);
				if (fdID == 0) {
					Serial.write("fdID is 0 \n");
					break;
				}
				numFood++;
				/*Serial.write("Genning food \n");*/
				curInventory->foodIList[curSlot] = curFoods.genFoods(0, fdID);
				DUMP(curSlot); 
				curSlot++;
				DUMP(ptr);
				ptr = strtok(NULL, delim);
			}
		}
	DUMP(curInventory->numOfFoods);
	DUMP(numFood); 
	curInventory->numOfFoods = numFood; 
	}

void gamePlay::removeChar(char* str, unsigned int index) {
	char* src;
	for (src = str + index; *src != '\0'; *src = *(src + 1), ++src);
	*src = '\0';
}

char* gamePlay::findInFile(String toFind, String fString) {
	// If string not empty Find where the required string starts For all chars in string If the cur char is not *
	// Add it to temp string, increment strI so next valid char goes in next slot 
	char tempStr[50] = "";
	char* ch = new char[50];
	/*String tempStr;*/
	/*int strI = 0;*/

	if (!fString.isEmpty()) {				
		int i = fString.indexOf(toFind);	
		i = i + 4;
		Serial.printf("Found index of %s at %d", toFind, i);
		if (i > -1)
		{
			for (i; i < fString.length(); i++) 
			{
				if (fString[i] != '*') {			
					char c = fString.charAt(i);
					strncat(tempStr, &c, 1);
				}
				else {
					break;
				}
			}
		}
	}
	strcpy(ch, tempStr);
	DUMP(ch);
	return ch;
}

int gamePlay::game_highlow() {
	bool isBlink = false; 
	int cardsLeft = 52; 
	char* cardRank; 
	bool isAce;
	char* cardRank2;
	unsigned short toDraw[625]; 
	unsigned short toDraw2[625];
	int card1x = 15;
	int card2x = 202;
	int cardy = 57;
	int cardw = 105;
	int cardh = 125; 
	game_HiLo ng_HiLo; 
	singleCard curCard; 
	singleCard nextCard; 
	m5.Lcd.drawPngFile(SD, "/bg/MG_HighLow.png", 0, 0);
	std::vector<singleCard> thisDeck = ng_HiLo.generateVDeck();
	bool isHigher = false; 
	bool waitForChoice = false; 

	ng_HiLo.curWinnings = 0;
	ng_HiLo.roundsPlayed = 0;
	while (!btnCPress) {
		aPlayer.playSound(scDrawCard);
		Serial.write("Doing next round \n"); 
		aPlayer.wavloop();
		isAce = false;
		DUMP(ng_HiLo.roundsPlayed); 
		if (ng_HiLo.roundsPlayed == 0) {
			int rando = (rand() % cardsLeft) + 1;
			curCard = thisDeck.at(rando);
			thisDeck.erase(thisDeck.begin() + (rando));
			cardsLeft--;
		}
		else {
			Serial.write("Cur card become next card \n"); 
			curCard = nextCard;
		}
		if (ng_HiLo.roundsPlayed >= 50) {
			//Holi crap they cleared the whole deck 
		}
		Serial.write(cardsLeft);
		Serial.write("\n");

		int rando = (rand() % cardsLeft) + 1;
		nextCard = thisDeck.at(rando);
		thisDeck.erase(thisDeck.begin() + (rando));
		cardsLeft--;
		tft.fillRect(card1x, cardy, cardw, cardh, TFT_WHITE);
		tft.fillRect(card2x, cardy, cardw, cardh, TFT_WHITE);
		DUMP(curCard.theSuit);
		DUMP(curCard.theVal);
		switch (curCard.theSuit) {
		case csHearts: memcpy(toDraw, symb_heart, sizeof(symb_heart)); break;
			case csClubs: memcpy(toDraw, symb_club, sizeof(symb_club)); break; 
			case csDiamonds: memcpy(toDraw, symb_diamond, sizeof(symb_diamond)); break; 
			case csSpades: memcpy(toDraw, symb_spade, sizeof(symb_spade)); break;
		}
		DUMP(nextCard.theSuit);
		DUMP(nextCard.theVal);
		switch (nextCard.theSuit) {
		case csHearts: memcpy(toDraw2, symb_heart, sizeof(symb_heart)); break;
		case csClubs: memcpy(toDraw2, symb_club, sizeof(symb_club)); break;
		case csDiamonds: memcpy(toDraw2, symb_diamond, sizeof(symb_diamond)); break;
		case csSpades: memcpy(toDraw2, symb_spade, sizeof(symb_spade)); break;
		}
		cardRank = ng_HiLo.cardRankNames[thisDeck[rando].theVal];
		cardRank2 = ng_HiLo.cardRankNames[nextCard.theVal];
		int xPos = 20;
		int yPos = 85; 
		for (int x = 0; x < 3; x++) {
			for (int y = 0; y < 3; y++) {
				tft.pushImage(xPos, yPos, 25, 25, toDraw);
				yPos = yPos + 25;
			}
			xPos = xPos + 30; 
			yPos = 85;
		}

		tft.fillRect(200, 55, 110, 130, TFT_BLUE); 
		tft.setTextSize(3);
		if (curCard.theSuit == csHearts || csClubs) {
			tft.setTextColor(TFT_RED);
		}
		else {
			tft.setTextColor(TFT_BLACK); 
		}
		tft.drawString(cardRank, 19, 60);
		tft.drawString(cardRank, 100, 160); 
		tft.setTextColor(TFT_BLACK);
		tft.setTextSize(2);
		char winChar[10]; 
		itoa(ng_HiLo.curWinnings, winChar, 10);
		tft.setTextColor(TFT_BLACK, 0x92A7);
		tft.drawString(winChar, 150, 30);
		if (curCard.theVal == cvAce) {
			isAce == true;
		}
		if (nextCard.theVal == cvAce) {
			isAce == true;
		}
		isHigher = ng_HiLo.isHigher(&curCard, &nextCard);
		waitForChoice = true;
		while (waitForChoice) {
			m5.update();
			aPlayer.wavloop(); 
			if (isBlink) {
				tft.pushImage(130, 120, 64, 64, blue_front, 0xFFFF);
				isBlink = false;
			}
			else {
				tft.pushImage(130, 120, 64, 64, blue_blink, 0xFFFF);
				isBlink = true; 
			}
			if (btnAPress || btnBPress) {
				// Reveal card
				tft.fillRect(200, 55, 110, 130, TFT_WHITE);
				int xPos = 200;
				int yPos = 85;
				for (int x = 0; x < 3; x++) {
					for (int y = 0; y < 3; y++) {
						tft.pushImage(xPos, yPos, 25, 25, toDraw2);
						yPos = yPos + 25;
					}
					xPos = xPos + 30;
					yPos = 85;
				}
				if (nextCard.theSuit == csHearts || csClubs) {
					tft.setTextColor(TFT_RED);
				}
				else {
					tft.setTextColor(TFT_BLACK);
				}
				tft.drawString(cardRank2, 250, 60);
				tft.drawString(cardRank2, 250, 160);
				Serial.write("Card revealed \n"); 
				if (btnAPress) {
					aPlayer.playSound(scButtonA);
					aPlayer.wavloop();
					clearButtons(); 
					Serial.write("User guess higher \n");
					//User guessed higher
					if (isHigher || isAce) {
						aPlayer.playSound(scWinGame);
						aPlayer.wavloop();
						Serial.write("User win \n");
						clearButtons();
						ng_HiLo.curWinnings = ng_HiLo.curWinnings + 10;
						waitForChoice = false; 
						delay(2000);
					}
					else {
						//Loser!!!!
						aPlayer.playSound(scLoseGame);
						aPlayer.wavloop();
						Serial.write("Loser \n");
						clearButtons();
						delay(2000);
						gameOverScreen(ng_HiLo.curWinnings, false);
						return ng_HiLo.curWinnings;
					}
				}
				if (btnBPress) {
					aPlayer.playSound(scButtonB);
					clearButtons();
					Serial.write("User guess low \n");
					//User guessed lower
					if (isHigher && !isAce) {
						aPlayer.playSound(scLoseGame);
						aPlayer.wavloop();
						Serial.write("User lose \n");
						clearButtons();
						//Loser!!!!
						//Do sad music
						delay(2000);
						gameOverScreen(ng_HiLo.curWinnings, false); 
						return ng_HiLo.curWinnings;
					}
					else if (!isHigher || isAce) {
						Serial.write("User win \n");
						clearButtons(); 
						aPlayer.playSound(scWinGame);
						aPlayer.wavloop();
						ng_HiLo.curWinnings = ng_HiLo.curWinnings + 10;
						waitForChoice = false; 
						delay(2000);
					}
				}
		}
	delay(500);
	}
	ng_HiLo.roundsPlayed++; 
	Serial.write("Next round"); 
	}
	if (btnCPress) {
		aPlayer.playSound(scButtonC);
		clearButtons();
		delay(200);
		aPlayer.forceStop();
		return ng_HiLo.curWinnings;
	}
	//aPlayer.forceStop();
	//return newGame.curWinnings;
}

int gamePlay::game_pong() {
	game_Pong ng_Pong;
	int playerScore = 0;
	int AIScore = 0;
	int scoreLimit = 10; 
	bool gameContinue = true; 

	int dashlineX = SCREEN_WIDTH / 2;
	int dashlineY = 0;
	int dashlineW = 20;
	int dashlineH = SCREEN_HEIGHT; 

	ball theBall;
	paddle paddlePlayer;
	paddle paddleAI;
	paddle paddleCurrent; 

	theBall.x = SCREEN_WIDTH / 2;
	theBall.y = SCREEN_HEIGHT / 2;
	theBall.w = 10;
	theBall.h = 10;
	theBall.dy = 1;
	theBall.dx = 1;

	paddlePlayer.x = 20;
	paddlePlayer.y = SCREEN_WIDTH / 2 - 50;
	paddlePlayer.w = 10;
	paddlePlayer.h = 50;

	paddleAI.x = SCREEN_HEIGHT - 10;
	paddleAI.y = SCREEN_HEIGHT / 2 - 50;
	paddleAI.w = 10;
	paddleAI.h = 50;
	tft.fillScreen(TFT_WHITE);

	while (gameContinue) {
		//Undraw previous circle/paddle
		tft.fillRect(paddleAI.x, paddleAI.y, paddleAI.w, 1, TFT_WHITE);
		tft.fillRect(paddlePlayer.x, paddlePlayer.y, paddlePlayer.w, 1, TFT_WHITE);
		tft.fillRect(theBall.x, theBall.y, theBall.w, theBall.h, TFT_WHITE);
		//beep, beep, beep, beeeep
		//Handle ball movement
		theBall.x += theBall.dx;
		theBall.y += theBall.dy;
		if (theBall.x < 0) {
			AIScore += 1;
			//beep, beep, beep, beeeep
			delay(3000); 
			theBall.x = SCREEN_WIDTH / 2;
			theBall.y = SCREEN_HEIGHT / 2;
			theBall.w = 10;
			theBall.h = 10;
			theBall.dy = 1;
			theBall.dx = 1;
		}
		if (theBall.x > SCREEN_WIDTH) {
			playerScore += 1;
			//beep, beep, beep, beeeep
			delay(3000);
			theBall.x = SCREEN_WIDTH / 2;
			theBall.y = SCREEN_HEIGHT / 2;
			theBall.w = 10;
			theBall.h = 10;
			theBall.dy = 1;
			theBall.dx = 1;
		}
		if (theBall.y < 0 || theBall.y > SCREEN_HEIGHT - 10) {
			theBall.dy = -theBall.dy;
		}
		int cp = ng_Pong.check_collision(theBall, paddlePlayer);
		int ca = ng_Pong.check_collision(theBall, paddleAI);
		if (ca == 1 || cp == 1) {
			if (ca == 1) {
				paddleCurrent = paddleAI;
			}
			else if (cp == 1) {
				paddleCurrent = paddlePlayer;
			}
			if (theBall.dx < 0) {
				theBall.dx -= 1;
			}
			else {
				theBall.dx += 1;
			}
			theBall.dx = -theBall.dx;
			//change ball angle based on where on the paddle it hit
			int hit_pos = (paddleCurrent.y + paddleCurrent.h) - theBall.y;
			if (hit_pos >= 0 && hit_pos < 7) {
				theBall.dy = 4;
			}

			if (hit_pos >= 7 && hit_pos < 14) {
				theBall.dy = 3;
			}

			if (hit_pos >= 14 && hit_pos < 21) {
				theBall.dy = 2;
			}

			if (hit_pos >= 21 && hit_pos < 28) {
				theBall.dy = 1;
			}

			if (hit_pos >= 28 && hit_pos < 32) {
				theBall.dy = 0;
			}

			if (hit_pos >= 32 && hit_pos < 39) {
				theBall.dy = -1;
			}

			if (hit_pos >= 39 && hit_pos < 46) {
				theBall.dy = -2;
			}

			if (hit_pos >= 46 && hit_pos < 53) {
				theBall.dy = -3;
			}

			if (hit_pos >= 53 && hit_pos <= 60) {
				theBall.dy = -4;
			}
		}
		//Handle AI move
		//ball moving down
		int center = paddleAI.y + 25;
		if (theBall.dy > 0) {
			if (theBall.y > center) {
				paddleAI.y += theBall.dy;
			}
			else {
				paddleAI.y -= theBall.dy;
			}
		}
		//ball moving up
		if (theBall.dy < 0) {
			if (theBall.y < center) {
				paddleAI.y -= theBall.dy;
			}
			else {
				paddleAI.y += theBall.dy;
			}
		}
		//ball moving stright across
		if (theBall.dy == 0) {
			if (theBall.y < center) {
				paddleAI.y -= 5;
			}
			else {
				paddleAI.y += 5;
			}
		}
		//Handle player paddle
		if (btnAPress) {
			clearButtons(); 
			//Ball move down
			if (paddlePlayer.y >= SCREEN_HEIGHT - paddlePlayer.h) {
				paddlePlayer.y = SCREEN_HEIGHT - paddlePlayer.h;
			}
			else {
				paddlePlayer.y += 5;
			}
		}
		if (btnBPress) {
			clearButtons();
			if (paddlePlayer.y <= 0) {
				paddlePlayer.y = 0;
			}
			else {
				paddlePlayer.y -= 5;
			}
		}

		//Draw stuff
		tft.startWrite();
		if ((theBall.x < dashlineX - theBall.w) && (theBall.x > dashlineX + dashlineW)) return 0;
		else {
			tft.setAddrWindow(dashlineX, 0, dashlineW, dashlineH);

			for (int16_t i = 0; i < 10; i += 2) {
				tft.pushColor(TFT_WHITE, dashlineW* dashlineH); // push dash pixels
				tft.pushColor(TFT_BLACK, dashlineW* dashlineH); // push gap pixels
			}
			tft.endWrite();
		}
		tft.fillRect(paddleAI.x, paddleAI.y, paddleAI.w, 1, TFT_BLACK);
		tft.fillRect(paddlePlayer.x, paddlePlayer.y, paddlePlayer.w, 1, TFT_BLACK);
		tft.fillRect(theBall.x, theBall.y, theBall.w, theBall.h, TFT_BLACK);
		//Check for game over
		if (AIScore >= scoreLimit) {
			gameContinue = false; 
			return 0; 
		}
		else if (playerScore >= scoreLimit) {
			gameContinue = false;
			return 0;
		}

		delay(10); 
		if (btnCPress) {
			aPlayer.playSound(scButtonC);
			clearButtons();
			delay(200);
			aPlayer.forceStop();
			gameContinue = false;
			return 0;
		}
	}
	if (btnCPress) {
		aPlayer.playSound(scButtonC);
		clearButtons();
		delay(200);
		aPlayer.forceStop();
		return 0;
	}

}

void gamePlay::gameOverScreen(int winnings, bool didWin) {
	char winStr[10];
	bool isCheer = false; 
	sprintf(winStr, "%d", winnings);
	tft.fillScreen(TFT_WHITE);
	tft.setTextColor(TFT_BLACK);
	tft.setTextSize(4);
	tft.drawString("GAME OVER", 60, 30);
	tft.setTextSize(3);
	tft.drawString("You won", 100, 80);
	tft.drawString(winStr, 140, 110);
	tft.drawString("Coins", 130, 140);
	tft.setTextSize(2);
	tft.drawString("OK", 250, 210);
	while (!btnCPress) {
		if (isCheer) {
			tft.pushImage(10, 160, 64, 64, blue_front, 0xFFFF);
			isCheer = false;
		}
		else {
			tft.pushImage(10, 160, 64, 64, blue_blink, 0xFFFF);
			isCheer = true;
		}
		delay(500); 
	}
	if (btnCPress) {
		aPlayer.playSound(scButtonC);
		clearButtons();
		aPlayer.forceStop();
		return;
	}
}
