#include <iostream>
#include <unordered_map>
#include <iomanip>
#include <utility>
#include <memory>
#include <vector>
using namespace std;
enum WEAPON_TYPE : int{
	SWORD = 0, BOMB = 1, ARROW = 2
};
enum WARRIOR_TYPE {
	DRAGON = 0, NINJA = 1, ICEMAN = 2, LION = 3, WOLF = 4
};
enum HEADQUARTER_TYPE {
	RED = -1, BLUE = 1
};

vector<int> BaseLife(5);
vector<int> BaseAttack(5);
unordered_map<int, string> hq2str = {{RED, "red"}, {BLUE,"blue"}};
unordered_map<int, string> wr2str = {{DRAGON, "dragon"}, {NINJA, "ninja"}, {WOLF, "wolf"}, {ICEMAN, "iceman"}, {LION, "lion"}};
unordered_map<int, vector<int>> warriorSeq = {
	{RED, {ICEMAN, LION, WOLF, NINJA, DRAGON}},
	{BLUE, {LION, DRAGON, NINJA, ICEMAN, WOLF}}
};

static int currentTime = 0;
static int M, N, R, K, T;
// R Arrow Atk, M HeadquarterLifeBase, N Cities Between, K loyalty reduction
class Warrior;
class Weapon;
class City;

static void printTimeWithBp() {
	int hr = currentTime / 60;
	int min = currentTime % 60;
	cout << setw(3) << setfill('0') << hr;
	cout << ":";
	cout << setw(2) << setfill('0') << min;
	cout << " ";
}

class Weapon {
protected:
	int number = 0;
	int attack = 0;
	int durability = -1;
public:
	virtual bool use() = 0; //return is_usable
	explicit Weapon(int num, int atk = 0, int dur= -1): number(num), attack(atk), durability(dur){};
	virtual ~Weapon() = default;
	virtual void report() = 0;
	friend class Warrior;
};

class Warrior {
protected:
	int number = 0;
	int attack = 0;
	int type = 0;
public: //位置仍未可知
	int blood = 0;
	int belonging = 0;
	int lifeBeforeFight = 0;
	unordered_map<int, Weapon*> weapons;
	Warrior(int num, int bel, int bld, int atk): number(num), belonging(bel),blood(bld),attack(atk) {
		weapons[ARROW] = nullptr;
		weapons[SWORD] = nullptr;
		weapons[BOMB] = nullptr;
	};
	//赋予武器可以在派生类构造里面写
	virtual int getType() = 0;
	virtual void reportWeapon();
	virtual bool hurt(int n) { //hurt -> true代表死亡
		blood -= n;
		if(blood <= 0) {
			blood = 0;
			return true;
		}
		return false;
	}
	void printSelf() const {
		cout << hq2str[belonging] << " " << wr2str[type] <<" " << number;
	}
	friend void printAny(Warrior* wr) {
		cout << hq2str[wr -> belonging] << " " << wr2str[wr -> type] <<" " << wr -> number;
	}
	bool is_alive() const { return blood > 0; }
	virtual void aftermath(bool is_winner, Warrior* enemy, City* city) = 0;
	virtual void march(City* city);
	void shoot(City* targetCity);
	bool sacrifice(City* city);
	virtual bool fight(City* city);
	virtual bool fight_back(City* city);
	void diedDue2Fight(City* city);
	void fetchCityLifePoint(City* city);
	void fetchWinningLifePoint(City* city);
	virtual ~Warrior() {
		for(auto wp : {ARROW, SWORD, BOMB}) {
			delete weapons[wp];
		}
	}; //一定注意什么时候析构
	friend class City;
};

struct SFightRecord {
	bool fight_happened = false;
	int winningSide = 0;
};
class City {
public:
	SFightRecord fight_record;
	int number = 0;
	int lifepoints = 0;
	bool is_headquarter = false;
	int move_first;
	int flagType;
	int prior_winning = 0;
	bool sacrificed;
	//TODO 更新注意计算平局,如果开始的时候双死（仅有可能是，就算平局
	unordered_map<int, Warrior*> warriors;
	City(int num, int life): number(num), lifepoints(life), warriors{{RED, nullptr}, {BLUE, nullptr}} {
		move_first = (num % 2 == 0) ? BLUE : RED;
		flagType = 0;
	};
	void generateLife() {
		lifepoints += 10;
	}
	void raiseflag(int type) {
		if(type == flagType) return;
		printTimeWithBp();
		move_first = type;
		flagType = type;
		cout<<hq2str[type] <<" flag raised in city " << number << endl;
	}
};

void Warrior::diedDue2Fight(City* city) {
	printTimeWithBp();
	printSelf();
	cout<<" was killed in city " << city -> number << endl;
}

class CArrow : public Weapon {
public:
	explicit CArrow(int atk = R):Weapon(ARROW, R, 3){};
	bool use() override {
		durability -= 1;
		if(durability == 0) {
			return false;
		}
		return true;
	}
	void report() override {
		cout<<"arrow("<<durability<<")";
	}
};
class CBomb : public Weapon {
public:
	explicit CBomb(int atk = 0):Weapon(BOMB, atk, 1){};
	bool use() override {
		return false;
	}
	void report() override {
		cout<<"bomb";
	}
};
class CSword : public Weapon {
public:
	explicit CSword(int atk):Weapon(SWORD, static_cast<int>(atk * 2 / 10) , -1){};
	bool use() override {
		attack = static_cast<int>(attack * 8 / 10);
		if(attack == 0) return false;
		return true;
	}
	void report() override {
		cout<<"sword("<<attack<<")";
	}
};

void Warrior::reportWeapon() {
	int weaponCnt = 0;
	for(auto types : {ARROW, BOMB, SWORD}) {
		if(weapons[types]) {
			weaponCnt++;
			if(types == SWORD && weapons[types] -> attack == 0) {
				delete weapons[types];
				weapons[types] = nullptr;
				weaponCnt--;
			}
		}
	}
	if(weaponCnt == 0) {
		printTimeWithBp();
		printSelf();
		cout<<" has no weapon" << endl;
		return;
	}
	printTimeWithBp();
	printSelf();
	cout<<" has ";
	for(auto types : {ARROW, BOMB, SWORD}) {
		if(weapons[types]) {
			weapons[types] -> report();
			weaponCnt--;
			if(weaponCnt > 0) cout<<',';
		}
	}
	cout<<endl;
}

void Warrior::shoot(City* targetCity){
	//shoot不判断自己是否死亡！TODO：确保先检测有无箭矢再输出
	if(targetCity -> is_headquarter) return;
	if(targetCity -> warriors[-belonging]) {
		Warrior* enemy = targetCity -> warriors[-belonging];
		printTimeWithBp();
		printSelf();
		cout << " shot";
		if(enemy -> hurt(weapons[ARROW] -> attack)) {
			cout << " " << "and killed ";
			printAny(enemy);
			cout << endl;
		} else {
			cout << endl;
		}
		if(!weapons[ARROW]->use()) {
			delete weapons[ARROW];
			weapons[ARROW] = nullptr;
		}
	}
}

class Headquarters : public City{
	int self_round = 0;
	int type = 0;
public:
	int enemyCnt = 0;
	int collected_lifepoints = 0;
	Warrior* temp = nullptr;
	//TODO:march行为注意判断
	explicit Headquarters(int num, int life, int type): type(type), City(num, life) {
		is_headquarter = true;
	};
	typedef pair<bool, Warrior*> make_warrior_ret;
	Warrior* generate_warrior(int curr_warrior) const;
	void make_warrior();
	bool reward(Warrior* wr) {
		if(lifepoints >= 8) {
			wr -> blood += 8;
			lifepoints -= 8;
			return true;
		}
		return false;
	}
	void collect_lifePoint() {
		lifepoints += collected_lifepoints;
		collected_lifepoints = 0;
	}
	void report_life() {
		printTimeWithBp();
		cout<< lifepoints << " elements in " << hq2str[type] << " headquarter" << endl;
	}
	bool is_taken() {
		bool ret = enemyCnt == 2;
		if(ret) {
			printTimeWithBp();
			cout << hq2str[type] << " headquarter was taken" << endl;
		}
		return ret;
	}
};

unordered_map<int, Headquarters*> MHeadquarters;
vector<City*> Cities;

void distribute_weapon(unordered_map<int, Weapon*>& weapons, int type, int atk) {
	switch(type) {
		case ARROW:
			weapons[ARROW] = new CArrow(R);
			break;
		case SWORD:
			weapons[SWORD] = new CSword(atk);
			break;
		case BOMB:
			weapons[BOMB] = new CBomb();
			break;
		default:
			break;
	}
}

class CIceman: public Warrior {
	int marched = 0;
public:
	CIceman(int num, int bel, int bld = BaseLife[ICEMAN], int atk = BaseAttack[ICEMAN]):Warrior(num, bel, bld, atk) {
		distribute_weapon(weapons, num % 3, atk);
		cout << hq2str[bel] << " " << "iceman" << " " << num << " " << "born" << endl;
		type = ICEMAN;
	};
	void march(City* targetCity) override {
		marched++;
		if(marched % 2 == 0) {
			blood = blood > 9 ? blood - 9 : 1;
			attack += 20;
		}
		printTimeWithBp();
		printSelf();
		if(targetCity -> is_headquarter) {
			string hq = (targetCity -> number == 0) ? hq2str[RED] : hq2str[BLUE];
			cout<<" reached " << hq << " headquarter with " <<
				blood << " elements and force " << attack << endl;
			static_cast<Headquarters*>(targetCity) -> enemyCnt++;
			return;
		}
		cout<<" marched to city " << targetCity -> number << " with " <<
				blood << " elements and force " << attack << endl;
	}
	int getType() override {return ICEMAN;}

	void aftermath(bool is_winner, Warrior* enemy, City* city) override {};
};
class CLion: public Warrior {
	int loyalty = 0;
public:
	CLion(int num, int bel, int bld = BaseLife[LION], int atk = BaseAttack[LION]):Warrior(num, bel, bld, atk) {
		this->loyalty = MHeadquarters[bel]->lifepoints;
		cout << hq2str[bel] << " " << "lion" << " " << num << " " << "born" << endl;
		cout << "Its loyalty is " << loyalty << endl;
		type = LION;
	}
	int getType() override {return LION;}
	void aftermath(bool is_winner, Warrior* enemy, City* city) override {
		if(!is_winner) {
			loyalty -= K;
		}
	}
	bool run_away() const { //return true 代表逃跑。可以delete [[nodiscard]]代表不能单独使用，必须接受返回值
		if(loyalty <= 0) {
			printTimeWithBp();
			cout << hq2str[belonging] << " " << "lion" << " " << number << " " << "ran away" << endl;
			return true;
		}
		return false;
	}
};
class CWolf: public Warrior {
	int loyalty = 0;
public:
	CWolf(int num, int bel, int bld = BaseLife[WOLF], int atk = BaseAttack[WOLF]):Warrior(num, bel, bld, atk) {
		cout << hq2str[bel] << " " << "wolf" << " " << num << " " << "born" << endl;
		type = WOLF;
	}
	int getType() override {return WOLF;}
	void aftermath(bool is_winner, Warrior* enemy, City* city) override {
		if(!is_winner) return;
		for(int i=0; i<=2; i++) {
			if(weapons[i]) continue;
			weapons[i] = enemy -> weapons[i];
			enemy -> weapons[i] = nullptr;
		}
	}
};
class CNinja: public Warrior{
public:
	CNinja(int num, int bel, int bld = BaseLife[NINJA], int atk = BaseAttack[NINJA]):Warrior(num, bel, bld, atk) {
		distribute_weapon(weapons, num % 3, atk);
		distribute_weapon(weapons, (num + 1) % 3, atk);
		cout << hq2str[bel] << " " << "ninja" << " " << num << " " << "born" << endl;
		type = NINJA;
	}
	int getType() override {return NINJA;}
	bool fight_back(City* city) override { return false; };
	void aftermath(bool is_winner, Warrior* enemy, City* city) override {};
};

class CDragon: public Warrior {
	double morale = 0.0;
public:
	CDragon(int num, int bel, int bld = BaseLife[DRAGON], int atk = BaseAttack[DRAGON]):Warrior(num, bel, bld, atk) {
		distribute_weapon(weapons, num % 3, atk);
		morale = static_cast<double>(MHeadquarters[bel]->lifepoints) / BaseLife[DRAGON];
		cout << hq2str[bel] << " " << "dragon" << " " << num << " " << "born" << endl;
		cout << "Its morale is " << fixed << setprecision(2) << morale << endl;
		type = DRAGON;
	}
	int getType() override {return DRAGON;}
	void yell(City* city) {
		if(city -> move_first != belonging) return;
		printTimeWithBp();
		printSelf();
		cout<<" yelled in city " << city -> number << endl;
	};
	void aftermath(bool is_winner, Warrior* enemy, City* city) override {
		if(is_winner) morale += 0.2;
		else morale -= 0.2;
		if(morale > 0.8) yell(city);
	}
};


bool Warrior::fight(City* city) { //返回值代表是否击杀敌方
	//TODO:如果一方已经死了，则判定胜利。城市判断两边都活着则进入fight程序。根据返回值输出killed时间以确保time sequence，判断是否进行反击
	//TODO:如果都活着，先判断sacrifice。
	if(city -> warriors[-belonging]) {
		if(!city -> warriors[-belonging] -> is_alive()) return true;
		Warrior* enemy = city -> warriors[-belonging];
		int real_atk = attack;
		if(weapons[SWORD]) {
			real_atk += weapons[SWORD]->attack;
			if(!weapons[SWORD] -> use()) {
				delete weapons[SWORD];
				weapons[SWORD] = nullptr;
			}
		}
		printTimeWithBp();
		printSelf();
		cout<<" attacked ";
		printAny(enemy);
		cout<<" in city " << city -> number << " ";
		cout<< "with " << blood << " elements and force " << attack << endl;
		bool winning = enemy->hurt(real_atk);
		if(enemy->getType() != LION) return winning;
		if(winning) blood += enemy -> lifeBeforeFight;
		return winning;
	}
	return false;
}
bool Warrior::fight_back(City* city) { //返回值代表是否击杀敌方
	int real_atk = attack / 2;
	if(weapons[SWORD]) {
		real_atk += weapons[SWORD]->attack;
		if(!weapons[SWORD] -> use()) {
			delete weapons[SWORD];
			weapons[SWORD] = nullptr;
		}
	}
	Warrior* enemy = city -> warriors[-belonging];
	printTimeWithBp();
	printSelf();
	cout<<" fought back against ";
	printAny(enemy);
	cout<<" in city " << city->number << endl;
	bool winning = enemy->hurt(real_atk);
	if(enemy->getType() != LION) return winning;
	if(winning) blood += enemy -> lifeBeforeFight;
	return winning;
}

Warrior* Headquarters::generate_warrior(int curr_warrior) const {
	switch(curr_warrior) {
		case DRAGON:
			return new CDragon(self_round, type);
		case LION:
			return new CLion(self_round, type);
		case ICEMAN:
			return new CIceman(self_round, type);
		case NINJA:
			return new CNinja(self_round, type);
		case WOLF:
			return new CWolf(self_round, type);
		default:
			break;
	}
	return nullptr;
}

void Headquarters::make_warrior() {
	int curr_warrior = warriorSeq[type][self_round % 5];
	int curr_warrior_cost = BaseLife[curr_warrior];
	if(lifepoints >= curr_warrior_cost) {
		printTimeWithBp();
		lifepoints -= curr_warrior_cost;
		self_round++;
		auto warrior = generate_warrior(curr_warrior);
		warriors[type] = warrior;
	}
}

void Warrior::fetchCityLifePoint(City* city) { //TODO:30分
	MHeadquarters[belonging]->lifepoints += city -> lifepoints;
	printTimeWithBp();
	printSelf();
	cout<<" earned " << city -> lifepoints << " elements for his headquarter" << endl;
	city -> lifepoints = 0;
}

void Warrior::fetchWinningLifePoint(City* city) { //TODO:40分
	MHeadquarters[belonging]->collected_lifepoints += city -> lifepoints;
	printTimeWithBp();
	printSelf();
	cout<<" earned " << city -> lifepoints << " elements for his headquarter" << endl;
	city -> lifepoints = 0;
}

void Warrior::march(City* targetCity) {
	//red iceman 1 marched to city 1 with 20 elements and force 30
	printTimeWithBp();
	printSelf();
	if(targetCity -> is_headquarter) {
		string hq = (targetCity -> number == 0) ? hq2str[RED] : hq2str[BLUE];
		cout<<" reached " << hq << " headquarter with " <<
			blood << " elements and force " << attack << endl;
		static_cast<Headquarters*>(targetCity) -> enemyCnt++;
		return;
	}
	cout<<" marched to city " << targetCity -> number << " with " <<
			blood << " elements and force " << attack << endl;
};

bool Warrior::sacrifice(City* city) { //是否成功
	Warrior* enemy = city -> warriors[-belonging];
	if(!enemy || !enemy -> is_alive() || !is_alive()) return false;
	if(city->move_first == belonging) {
		int real_atk = attack;
		if(weapons[SWORD]) {
			real_atk += weapons[SWORD]->attack;
		}
		if(real_atk >= enemy -> blood) return false;
		if(enemy->getType()==NINJA) return false;
		int enemy_lower_bound = enemy -> attack / 2;
		if(enemy->weapons[SWORD]) {
			enemy_lower_bound += enemy->weapons[SWORD]->attack;
		}
		if(enemy_lower_bound >= blood) {
			printTimeWithBp();
			printSelf();
			cout<<" used a bomb and killed ";
			printAny(enemy);
			cout << endl;
			blood = 0;
			enemy -> blood = 0;
			return true;
		}
	}
	if(city->move_first != belonging) {
		int enemy_lower_bound = enemy->attack;
		if(enemy->weapons[SWORD]) {
			enemy_lower_bound += enemy->weapons[SWORD]->attack;
		}
		if(enemy_lower_bound >= blood) {
			printTimeWithBp();
			printSelf();
			cout<<" used a bomb and killed ";
			printAny(enemy);
			cout << endl;
			blood = 0;
			enemy -> blood = 0;
			return true;
		}
	}
	return false;
}

class fullGame {
public:
	explicit fullGame() {
		cin >> M >> N >> R >> K >> T;
		//getBaseLife
		for(int i=0; i < 5; i++) {
			cin >> BaseLife[i];
		}
		//getBaseAttack
		for(int i=0; i < 5; i++) {
			cin >> BaseAttack[i];
		}
	}
	void reset() {
		for(auto city : Cities) {
			delete city;
		}
		Cities.clear();
		MHeadquarters.clear();
	}
	void run() {
		currentTime = 0;
		MHeadquarters[RED] = new Headquarters(0, M, RED);
		MHeadquarters[BLUE] = new Headquarters(N+1, M, BLUE);
		Cities.push_back(MHeadquarters[RED]);
		for(int i=1;i<=N;i++) {
			Cities.push_back(new City(i,0));
		}
		Cities.push_back(MHeadquarters[BLUE]);
		bool ongoing = true;
		auto belType = {RED,BLUE};
		while(currentTime <= T) {

			//在每个小时的第0分，制造士兵
			for(auto cType : belType) {
				MHeadquarters[cType] -> make_warrior();
			}
			currentTime += 5;
			if(currentTime > T) break;
			//在每个小时的第5分，该逃跑的lion就在这一时刻逃跑了。
			for(auto city : Cities) {
				for(auto cType : belType) {
					Warrior*& wr = city -> warriors[cType];
					if(wr && wr -> getType() == LION) {
						if(static_cast<CLion*>(wr) -> run_away()) {
							delete wr;
							city -> warriors[cType] = nullptr;
						}
					}
				}
				//判断
			}
			currentTime += 5;
			if(currentTime > T) break;
			//前进
			if(Cities[N+1] -> warriors[RED]) MHeadquarters[BLUE]->temp = Cities[N+1] -> warriors[RED];
			if(Cities[0] -> warriors[BLUE]) MHeadquarters[RED]->temp = Cities[0] -> warriors[BLUE];
			for(int i=0;i<=N;i++) {
				//红方：0 -> N 蓝方 N -> 0 先实质上移动，再调用march
				Cities[N + 1 - i] -> warriors[RED] = Cities[N - i] -> warriors[RED];
				Cities[i] -> warriors[BLUE] = Cities[i + 1] -> warriors[BLUE];
				//TODO：每次调用march的时候更新enemyCnt
			}
			Cities[0] -> warriors[RED] = nullptr;
			Cities[N + 1] -> warriors[BLUE] = nullptr;
			bool redTaken, blueTaken;
			for(int i=0;i<=N+1;i++) {
				for(auto cType : belType) {
					Warrior*& cWr = Cities[i] -> warriors[cType];
					if(!cWr) continue;
					cWr -> march(Cities[i]);
				}
				if(i == 0) redTaken = MHeadquarters[RED] -> is_taken();
				if(i == N+1) blueTaken = MHeadquarters[BLUE] -> is_taken();
			}
			ongoing = !redTaken && !blueTaken;
			if(!ongoing) break;
			//在每个小时的第20分：每个城市产出10个生命元。
			currentTime += 10;
			for(int i=1;i<=N;i++) {
				Cities[i] -> generateLife();
			}
			currentTime += 10;
			if(currentTime > T) break;
			//在每个小时的第30分：如果某个城市中只有一个武士，那么该武士取走该城市中的所有生命元
			for(int i=1;i<=N;i++) {
				int Cnt{0};
				for(auto cType : belType) {
					if(Cities[i] -> warriors[cType]) Cnt++;
				}
				if(Cnt == 1) {
					for(auto cType : belType) {
						if(Cities[i] -> warriors[cType]) {
							Cities[i] -> warriors[cType] -> fetchCityLifePoint(Cities[i]);
							break;
						}
					}
				}
			}
			currentTime += 5;
			if(currentTime > T) break;
			//在每个小时的第35分，拥有arrow的武士放箭
			for(int i=0;i<=N+1;i++) {
				for(auto cType : belType) {
					if(cType == RED && i == N+1) continue;
					if(cType == BLUE && i == 0) continue;
					if(Cities[i] -> warriors[cType] && Cities[i] -> warriors[cType] -> weapons[ARROW]){
						Warrior*& cWr = Cities[i] -> warriors[cType];
						cWr -> shoot(Cities[i - cWr -> belonging]);
					}
				}
			}
			currentTime += 3;
			if(currentTime > T) break;
			//在每个小时的第38分，拥有bomb的武士评估是否应该使用bomb
			for(int i=0;i<=N+1;i++) {
				int move_first = Cities[i] -> move_first;
				bool killed = false;
				if(Cities[i] -> warriors[move_first] && Cities[i] -> warriors[move_first] -> weapons[BOMB]){
					killed = Cities[i] -> warriors[move_first] -> sacrifice(Cities[i]);
				}
				if(!killed && Cities[i] -> warriors[-move_first] && Cities[i] -> warriors[-move_first] -> weapons[BOMB]){
					Cities[i] -> warriors[-move_first] -> sacrifice(Cities[i]);
				}
			}
			currentTime+=2;
			if(currentTime > T) break;
			//在每个小时的第40分：在有两个武士的城市，会发生战斗。条件是：两个位置都有武士，且至少有一个位置的武士is_alive
			//TODO：需要结算死亡情况 finished
			for(int i=0;i<=N+1;i++) {
				for(auto cType : belType) {
					if(Cities[i]->warriors[cType] && Cities[i]->warriors[cType] -> getType() == LION) {
						Cities[i]->warriors[cType]->lifeBeforeFight = Cities[i]->warriors[cType]->blood;
					}
				}
			}
			for(int i=0;i<=N+1;i++) {
				int alive = 0;
				int exist = 0;
				bool killed = false;
				auto& currCity = Cities[i];
				SFightRecord& fightRecord = currCity -> fight_record;
				fightRecord.winningSide = 0;
				fightRecord.fight_happened = false;
				for(auto cType : belType) {
					if(currCity -> warriors[cType]) {
						exist++;
						if(currCity -> warriors[cType] -> is_alive()) {
							alive++;
						}
					}
				}
				if(exist == 2 && alive == 1) {//活着的自动胜利，直接调用aftermath即可
					fightRecord.fight_happened = true;
					for(auto cType : belType) {
						Warrior*& enemy = currCity -> warriors[-cType];
						if(currCity -> warriors[cType] && currCity -> warriors[cType] -> is_alive()) {
							currCity -> warriors[cType] -> aftermath(true, enemy, currCity);
							currCity -> warriors[cType] -> fetchWinningLifePoint(currCity);
							 //可优化，懒得重写了. TODO:此处Dragon的yell依赖currCity的move_first参数，务必等aftermath结束后再修改move_first
							fightRecord.winningSide = cType;
							break;
						}
					}
				}
				if(exist == 2 && alive == 2) {//发生正式战斗
					fightRecord.fight_happened = true;
					int move_first = currCity -> move_first;
					auto& wrs = currCity -> warriors;
					killed = Cities[i] -> warriors[move_first] -> fight(Cities[i]);
					if(killed) {
						fightRecord.winningSide = move_first;
						wrs[-move_first] -> diedDue2Fight(Cities[i]);
						currCity -> warriors[move_first] -> aftermath(true, currCity -> warriors[-move_first], currCity);
					}
					if(!killed){
						killed = Cities[i] -> warriors[-move_first] -> fight_back(Cities[i]);
						if(killed) {
							fightRecord.winningSide = -move_first;
							wrs[move_first] -> diedDue2Fight(Cities[i]);
							currCity -> warriors[-move_first] -> aftermath(true, currCity -> warriors[move_first], currCity);
						}
						if(!killed) {
							fightRecord.winningSide = 0;
							currCity -> warriors[-move_first] -> aftermath(false, currCity -> warriors[move_first], currCity);
							currCity -> warriors[move_first] -> aftermath(false, currCity -> warriors[move_first], currCity);
						}
					}
					if(fightRecord.winningSide != 0) currCity -> warriors[fightRecord.winningSide] -> fetchWinningLifePoint(currCity);
					//TODO:aftermath finished
				}
				//开始调整旗帜和move_first
				if(fightRecord.fight_happened) {
					if(fightRecord.winningSide == currCity -> prior_winning && fightRecord.winningSide != 0) currCity -> raiseflag(fightRecord.winningSide);
					currCity -> prior_winning = fightRecord.winningSide;
				}
			}
			//统一析构死亡者!
			for(int i=1;i<=N;i++) {
				for(auto cType : belType) {
					if(Cities[i]->warriors[cType] && !Cities[i]->warriors[cType]->is_alive()) {
						delete Cities[i]->warriors[cType];
						Cities[i]->warriors[cType] = nullptr;
					}
				}
			}
			//TODO：两个方向分别发奖励
			bool blue_available = true;
			bool red_available = true;
			for(int i=1;i<=N;i++) {
				if(!blue_available && !red_available) break;
				if(blue_available && Cities[i]->fight_record.fight_happened && Cities[i]->fight_record.winningSide == BLUE) {
					blue_available = MHeadquarters[BLUE]->reward(Cities[i]->warriors[BLUE]);
				}
				if(red_available && Cities[i]->fight_record.fight_happened && Cities[i]->fight_record.winningSide == RED) {
					red_available = MHeadquarters[RED]->reward(Cities[i]->warriors[RED]);
				}
			}
			//TODO：fetchWinningLifePoints
			MHeadquarters[RED] -> collect_lifePoint();
			MHeadquarters[BLUE] -> collect_lifePoint();
			currentTime += 10;
			if(currentTime > T) break;
			MHeadquarters[RED] -> report_life();
			MHeadquarters[BLUE] -> report_life();
			currentTime += 5;
			if(currentTime > T) break;
			for(int i=0;i<=N+1;i++) {
				if(Cities[i]->warriors[RED]) Cities[i]->warriors[RED]->reportWeapon();
			}
			if(MHeadquarters[BLUE]->temp) MHeadquarters[BLUE]->temp -> reportWeapon();
			if(MHeadquarters[RED]->temp) MHeadquarters[RED]->temp -> reportWeapon();
			for(int i=0;i<=N+1;i++) {
				if(Cities[i]->warriors[BLUE]) Cities[i]->warriors[BLUE]->reportWeapon();
			}
			currentTime += 5;
			if(currentTime > T) break;
		}
		reset();
	}
};

int main() {
	int caseCount = 0;
	cin >> caseCount;
	for(int i = 1; i <= caseCount ; i++) {
		fullGame currentGame{};
		cout << "Case " << i << ':' << endl;
		currentGame.run();
	}
	//注意输入BaseLife和BaseAttack
	return 0;
}
