#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define EXPORT extern "C" EMSCRIPTEN_KEEPALIVE
#else
#define EXPORT extern "C"
#endif

static std::string trimStr(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(start, end - start);
}

static std::string toLowerStr(const std::string& s) {
    std::string out = s;
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

class Player;

class Item {
protected:
    std::string name;
    std::string description;
public:
    Item(std::string n, std::string desc) : name(n), description(desc) {}
    virtual ~Item() = default;
    std::string getName() const { return name; }
    std::string getDescription() const { return description; }
    virtual bool use(Player& player, std::ostringstream& out) = 0;
};

class TorchItem : public Item {
public:
    TorchItem() : Item("Torch", "Flare Torch: illuminates dark maze wings, restores +35 STM on use & sears demons.") {}
    bool use(Player& player, std::ostringstream& out) override;
};

class HerbItem : public Item {
private:
    int healAmount;
public:
    HerbItem(int amt = 45) : Item("Herb", "Crimson Herb: restores vitality during deep maze runs."), healAmount(amt) {}
    bool use(Player& player, std::ostringstream& out) override;
};

class BladeItem : public Item {
public:
    BladeItem() : Item("Blade", "Level Blade/Glaive/Scythe: boosts melee strike power across the maze.") {}
    bool use(Player&, std::ostringstream& out) override {
        out << "\n[Weapon] Your level weapon is equipped and ready!\n";
        return false;
    }
};

class KeyItem : public Item {
public:
    KeyItem() : Item("Key", "Sanctum Seal Key: unlocks the level's Final Gate.") {}
    bool use(Player&, std::ostringstream& out) override {
        out << "\n[Key] Reach the Final Gate and use 'door' to unlock it.\n";
        return false;
    }
};

class RelicItem : public Item {
public:
    RelicItem() : Item("Relic", "Ancient Relic: channels raw arcane/eclipse power.") {}
    bool use(Player&, std::ostringstream& out) override {
        out << "\n[Relic] The Relic pulses with ancient energy!\n";
        return false;
    }
};

class Inventory {
private:
    std::vector<Item*> items;
public:
    void reset() { items.clear(); }
    bool hasItem(const std::string& itemName) const {
        std::string target = toLowerStr(itemName);
        for (Item* it : items) {
            if (toLowerStr(it->getName()) == target) return true;
        }
        return false;
    }
    Item* getItem(const std::string& itemName) {
        std::string target = toLowerStr(itemName);
        for (Item* it : items) {
            if (toLowerStr(it->getName()) == target) return it;
        }
        return nullptr;
    }
    void addItem(Item* item, std::ostringstream& out) {
        if (!item) return;
        if (hasItem(item->getName())) {
            out << "\n[Pack] You already carry [" << item->getName() << "].\n";
            return;
        }
        items.push_back(item);
        out << "\n[Acquired] Added [" << item->getName() << "] — " << item->getDescription() << "\n";
    }
    void removeItem(const std::string& itemName) {
        std::string target = toLowerStr(itemName);
        items.erase(std::remove_if(items.begin(), items.end(),
            [&](Item* it) { return toLowerStr(it->getName()) == target; }), items.end());
    }
};

class Entity {
protected:
    std::string name;
    int health;
    int maxHealth;
public:
    Entity(std::string n, int hp) : name(n), health(hp), maxHealth(hp) {}
    virtual ~Entity() = default;
    std::string getName() const { return name; }
    int getHealth() const { return health; }
    int getMaxHealth() const { return maxHealth; }
    bool isAlive() const { return health > 0; }
    void setStats(std::string n, int hp) { name = n; health = hp; maxHealth = hp; }
    virtual void takeDamage(int amount, std::ostringstream& out) {
        health = std::max(0, health - amount);
        out << " [!] " << name << " takes " << amount << " DMG (" << health << "/" << maxHealth << " HP)\n";
    }
    virtual void heal(int amount, std::ostringstream& out) {
        health = std::min(maxHealth, health + amount);
        out << " [+] " << name << " recovers +" << amount << " HP (" << health << "/" << maxHealth << " HP)\n";
    }
};

class Player : public Entity {
private:
    Inventory inventory;
    int baseAttack;
    int avatarId;
    int stamina;
    int maxStamina;
public:
    Player() : Entity("Kaelen", 140), baseAttack(24), avatarId(1), stamina(100), maxStamina(100) {}

    void configureAvatar(int id) {
        avatarId = id;
        inventory.reset();
        stamina = 100;
        maxStamina = 100;
        if (id == 2) {
            setStats("Lyra", 125);
            baseAttack = 30;
        } else if (id == 3) {
            setStats("Thorne", 160);
            baseAttack = 22;
        } else {
            avatarId = 1;
            setStats("Kaelen", 140);
            baseAttack = 24;
        }
    }

    int getAvatarId() const { return avatarId; }
    int getStamina() const { return stamina; }
    int getMaxStamina() const { return maxStamina; }

    bool spendStamina(int cost) {
        if (stamina < cost) return false;
        stamina -= cost;
        return true;
    }

    void drainMoveStamina(int cost, std::ostringstream& out) {
        stamina = std::max(0, stamina - cost);
        if (stamina == 0) {
            out << " [Exhausted] Stamina reached 0! Moving while exhausted costs 10 HP!\n";
            takeDamage(10, out);
        }
    }

    void gainStamina(int amount) {
        stamina = std::min(maxStamina, stamina + amount);
    }

    Inventory& getInventory() { return inventory; }
    const Inventory& getInventory() const { return inventory; }

    int getAttackPower(int levelNumber) const {
        int total = baseAttack + (levelNumber - 1) * 5;
        if (inventory.hasItem("Blade")) total += 18 + (levelNumber * 4);
        if (inventory.hasItem("Torch")) total += 7;
        if (inventory.hasItem("Relic")) total += (levelNumber == 3 ? 32 : 20);
        return total;
    }

    void useItem(const std::string& itemName, std::ostringstream& out) {
        Item* item = inventory.getItem(itemName);
        if (!item) {
            out << "\n[!] No '" << itemName << "' in your pack.\n";
            return;
        }
        if (item->use(*this, out)) {
            inventory.removeItem(itemName);
        }
    }
};

bool TorchItem::use(Player& player, std::ostringstream& out) {
    player.gainStamina(35);
    out << "\n[Torch] You flare the Torch to steady your breathing (+35 STM).\n";
    return false;
}

bool HerbItem::use(Player& player, std::ostringstream& out) {
    player.heal(healAmount, out);
    return true;
}

class Enemy : public Entity {
protected:
    int attackDamage;
    std::string taunt;
    std::string villainKind;
public:
    Enemy(std::string n, std::string kind, int hp, int dmg, std::string t)
        : Entity(n, hp), attackDamage(dmg), taunt(t), villainKind(kind) {}
    const std::string& getKind() const { return villainKind; }
    int getAttackDamage() const { return attackDamage; }
    void resetEnemy(int hp, int dmg, std::string t) {
        health = hp; maxHealth = hp; attackDamage = dmg; taunt = t;
    }
    virtual void strikePlayer(Player& player, std::ostringstream& out, bool guarded = false) {
        out << " [" << name << "] \"" << taunt << "\"\n";
        int dmg = guarded ? std::max(6, attackDamage / 2) : attackDamage;
        if (guarded) out << " [Guard] Your stance cuts the strike in half!\n";
        player.takeDamage(dmg, out);
    }
};

class DemonMonkeyVillain : public Enemy {
public:
    DemonMonkeyVillain() : Enemy("Demon Monkey", "monkey", 66, 19, "Screech! The maze wings belong to us!") {}
};
class ShadowWraithVillain : public Enemy {
public:
    ShadowWraithVillain() : Enemy("Shadow Wraith", "wraith", 82, 23, "Cold mist drains wandering delvers!") {}
};
class IronGolemVillain : public Enemy {
public:
    IronGolemVillain() : Enemy("Obsidian Golem", "golem", 98, 26, "Stone and fury guard this wing!") {}
};
class AstralSentinelVillain : public Enemy {
public:
    AstralSentinelVillain() : Enemy("Astral Warden", "seraph", 118, 30, "None pass the gate unchallenged!") {}
};

class Room {
private:
    std::string id;
    std::string title;
    std::string objectiveHint;
    std::string description;
    Item* roomItem;
    Enemy* enemy;
    Room* northRoom;
    Room* southRoom;
    Room* eastRoom;
    Room* westRoom;
    bool deadEndRoom;
    bool lethalWrongTurn;
    std::string lethalMsg;
    std::string riddlePrompt;
    std::string riddleAnswer;
    bool riddleSolved;
public:
    Room(std::string roomId)
        : id(roomId), roomItem(nullptr), enemy(nullptr),
          northRoom(nullptr), southRoom(nullptr), eastRoom(nullptr), westRoom(nullptr),
          deadEndRoom(false), lethalWrongTurn(false), riddleSolved(false) {}

    const std::string& getId() const { return id; }
    const std::string& getTitle() const { return title; }
    const std::string& getObjective() const { return objectiveHint; }
    bool isDeadEnd() const { return deadEndRoom; }
    bool isLethal() const { return lethalWrongTurn; }
    const std::string& getLethalMsg() const { return lethalMsg; }
    bool hasRiddle() const { return !riddlePrompt.empty(); }
    bool isRiddleSolved() const { return riddleSolved || riddlePrompt.empty(); }
    const std::string& getRiddlePrompt() const { return riddlePrompt; }

    bool solveRiddle(const std::string& ans, Player& player, std::ostringstream& out) {
        std::string c = toLowerStr(trimStr(ans));
        if (c == riddleAnswer) {
            riddleSolved = true;
            out << "\n[Solved] The rune lock disengages!\n";
            return true;
        }
        out << "\n[Wrong Answer] Trap runes discharge for 20 DMG!\n";
        player.takeDamage(20, out);
        return false;
    }

    void setRoomData(std::string t, std::string hint, std::string desc, Item* item, Enemy* foe,
                     bool deadEnd = false, bool lethal = false, std::string lMsg = "",
                     std::string rPrompt = "", std::string rAns = "") {
        title = t; objectiveHint = hint; description = desc;
        roomItem = item; enemy = foe;
        deadEndRoom = deadEnd; lethalWrongTurn = lethal; lethalMsg = lMsg;
        riddlePrompt = rPrompt; riddleAnswer = toLowerStr(rAns);
        riddleSolved = riddlePrompt.empty();
    }

    void setExits(Room* n, Room* s, Room* e, Room* w) {
        northRoom = n; southRoom = s; eastRoom = e; westRoom = w;
    }

    Room* getNorth() const { return northRoom; }
    Room* getSouth() const { return southRoom; }
    Room* getEast() const { return eastRoom; }
    Room* getWest() const { return westRoom; }
    Enemy* getEnemy() const { return enemy; }

    Item* takeRoomItem() {
        Item* temp = roomItem;
        roomItem = nullptr;
        return temp;
    }

    void describe(std::ostringstream& out, int hp, int maxHp, int stm) const {
        out << "\n[" << title << "] HP " << hp << "/" << maxHp << " | STM " << stm << " — " << objectiveHint << "\n";
        out << description << "\n";
        std::string exits = "Exits:";
        if (northRoom) exits += " [North]";
        if (southRoom) exits += " [South]";
        if (eastRoom)  exits += " [East]";
        if (westRoom)  exits += " [West]";
        out << " " << exits << "\n";
        if (roomItem) out << " * Item/Weapon: [" << roomItem->getName() << "] ('take')\n";
        if (enemy && enemy->isAlive()) {
            out << " * Demon/Guardian: " << enemy->getName() << " (" << enemy->getHealth() << "/" << enemy->getMaxHealth() << " HP)\n";
        }
        if (!riddlePrompt.empty() && !riddleSolved) {
            out << " * Seal Riddle ('search'): " << riddlePrompt << "\n";
        }
    }
};

class GameEngine {
private:
    Player player;
    TorchItem torchItem;
    BladeItem bladeItem;
    HerbItem herbItem1;
    HerbItem herbItem2;
    KeyItem keyItem;
    RelicItem relicItem;

    DemonMonkeyVillain monkeyEnemy;
    ShadowWraithVillain wraithEnemy;
    IronGolemVillain golemEnemy;
    AstralSentinelVillain sentinelEnemy;

    // Extensive 9-room maze across North, South, East, and West wings
    Room rStart, rWest1, rWest2, rEast1, rEast2, rNorth1, rNorth2, rSouthVault, rGate;
    Room* currentRoom;
    int levelNumber;
    int defeatsCount;
    bool worldDestroyed;
    bool awaitingRiddle;
    std::string lastAnimAction;

    void wireMaze() {
        // Central hub rStart (courtyard): North -> rNorth1, East -> rEast1, West -> rWest1, South -> rSouthVault
        // West wing: rStart <-> rWest1 (West weapon/demon) <-> rWest2 (West deeper demon & dead end)
        // East wing: rStart <-> rEast1 (East weapon/demon) <-> rEast2 (East deeper demon & lethal trap)
        // North wing: rStart <-> rNorth1 (Gallery) <-> rNorth2 (Relic Sanctum) <-> rGate (Final Gate)
        // South wing: rStart <-> rSouthVault (extra stamina drain if running South)
        rStart.setExits(&rNorth1, &rSouthVault, &rEast1, &rWest1);
        rWest1.setExits(nullptr, nullptr, &rStart, &rWest2);
        rWest2.setExits(nullptr, nullptr, &rWest1, nullptr);
        rEast1.setExits(nullptr, nullptr, &rEast2, &rStart);
        rEast2.setExits(nullptr, nullptr, nullptr, &rEast1);
        rNorth1.setExits(&rNorth2, &rStart, &rEast1, &rWest1);
        rNorth2.setExits(&rGate, &rNorth1, &rEast2, &rWest2);
        rSouthVault.setExits(&rStart, nullptr, &rEast1, &rWest1);
        rGate.setExits(nullptr, &rNorth2, nullptr, nullptr);
    }

    void restartCurrentLevel(const std::string& reason, std::ostringstream& out) {
        defeatsCount++;
        lastAnimAction = "defeat";
        out << "\n[LEVEL RESTARTED] " << reason << "\n";
        configureLevel(levelNumber, out);
    }

public:
    GameEngine()
        : herbItem1(45), herbItem2(50),
          rStart("cell"), rWest1("west_hall"), rWest2("west_crypt"),
          rEast1("east_forge"), rEast2("east_abyss"),
          rNorth1("corridor"), rNorth2("sanctum"), rSouthVault("cistern"), rGate("gate"),
          currentRoom(&rStart), levelNumber(1), defeatsCount(0),
          worldDestroyed(false), awaitingRiddle(false), lastAnimAction("idle") {
        wireMaze();
    }

    void configureLevel(int lvl, std::ostringstream& out) {
        levelNumber = std::max(1, std::min(3, lvl));
        player.configureAvatar(player.getAvatarId());
        awaitingRiddle = false;
        worldDestroyed = false;
        lastAnimAction = "idle";
        currentRoom = &rStart;

        if (levelNumber == 1) {
            monkeyEnemy.resetEnemy(66, 19, "Screech! The ruins test every turn!");
            wraithEnemy.resetEnemy(80, 22, "Mist chills the western & southern halls!");
            golemEnemy.resetEnemy(96, 25, "Stone guards the eastern & northern wings!");
            sentinelEnemy.resetEnemy(112, 28, "Only worthy delvers breach the gate!");

            rStart.setRoomData("1. Sunlit Courtyard",
                "Lv.1 Objective: Explore North, East & West wings, claim weapons/Key/Relic & unlock Gate.",
                "Maze crossroads. Go West for a weapon & demon, East for trials & danger, North toward the Gate.",
                &torchItem, nullptr);
            rWest1.setRoomData("2. Western Armory Wing",
                "Claim the level Blade in the West wing & defeat the Demon Monkey!",
                "Torchlit western gallery holding a weapon and a prowling Demon Monkey.",
                &bladeItem, &monkeyEnemy);
            rWest2.setRoomData("3. Western Dead-End Crypt",
                "Dead end! Claim Herb, fight Wraith, then turn East to rejoin the maze.",
                "A deep western dead-end crypt haunted by a Shadow Wraith.",
                &herbItem1, &wraithEnemy, true, false);
            rEast1.setRoomData("4. Eastern Ember Wing",
                "Eastern wing: Defeat Obsidian Golem & solve the eastern rune riddle ('search').",
                "Glowing eastern chamber where a demon guards the deeper eastern passage.",
                &herbItem2, &golemEnemy, false, false, "",
                "What gets wetter the more it dries?", "towel");
            rEast2.setRoomData("5. Eastern Chasm (Lethal Turn)",
                "Lethal wrong turn! Turn West if you survived!",
                "Jagged collapse at the far eastern edge!",
                nullptr, nullptr, false, true, "Lethal wrong turn in the deep Eastern Chasm!");
            rNorth1.setRoomData("6. Northern Shifting Gallery",
                "Defeat Demon Monkey & push North toward the Relic Sanctum.",
                "Northern hall lined with dart slits and statues.",
                nullptr, &monkeyEnemy);
            rNorth2.setRoomData("7. Northern Relic Sanctum",
                "Claim [Key] & [Relic], defeat Golem, advance North to Final Gate.",
                "Sanctum holding the Key and Ancient Relic.",
                &keyItem, &golemEnemy);
            rSouthVault.setRoomData("8. Southern Sunken Vault (Extra STM Drain)",
                "South wing drains extra stamina! Claim Relic & return North.",
                "Flooded southern vault—running South takes heavy stamina.",
                &relicItem, &wraithEnemy);
            rGate.setRoomData("9. Level 1 Exit Gate",
                "Use 'door' with Key to advance to Level 2.",
                "Massive northern gate sealed with ancient runes.",
                nullptr, &sentinelEnemy);
        } else if (levelNumber == 2) {
            monkeyEnemy.resetEnemy(76, 23, "Relic hunters face fierce demons in every wing!");
            wraithEnemy.resetEnemy(90, 26, "Wraiths stalk the West & South corridors!");
            golemEnemy.resetEnemy(108, 29, "Golems guard the East & North vaults!");
            sentinelEnemy.resetEnemy(126, 32, "The Warden bars the Level 2 Gate!");

            rStart.setRoomData("1. Relic Crossroads",
                "Lv.2 Objective: Claim Arcane Glaive (West) & Ancient Relic, then unlock Level 2 Gate.",
                "Deeper labyrinth. Weapons and demons lurk in West, East, and North.",
                &torchItem, nullptr);
            rWest1.setRoomData("2. West Glaive Chamber",
                "Claim Arcane Glaive [Blade] & defeat Demon Monkey!",
                "Western chamber where a demon guards a sharp glaive.",
                &bladeItem, &monkeyEnemy);
            rWest2.setRoomData("3. West Mist Dead End",
                "Dead end! Fight Shadow Wraith & head East.",
                "Dead-end western mist chamber.",
                &herbItem1, &wraithEnemy, true, false);
            rEast1.setRoomData("4. East Demon Foundry",
                "Defeat Obsidian Golem & solve the mirror riddle ('search').",
                "Eastern foundry with a demon guardian and rune lock.",
                &relicItem, &golemEnemy, false, false, "",
                "I speak without a mouth and hear without ears. What am I?", "echo");
            rEast2.setRoomData("5. East Pendulum Trap (Lethal)",
                "Lethal wrong turn!",
                "Swinging blades strike across the far-east passage!",
                nullptr, nullptr, false, true, "Lethal wrong turn into the East Pendulum Trap!");
            rNorth1.setRoomData("6. North Mirror Corridor",
                "Defeat Demon Monkey & advance North.",
                "Northern corridor with shifting mirrors.",
                nullptr, &monkeyEnemy);
            rNorth2.setRoomData("7. North Relic Altar",
                "Claim [Key] & [Relic], defeat Golem, push North to Gate.",
                "Northern altar chamber.",
                &keyItem, &golemEnemy);
            rSouthVault.setRoomData("8. South Cistern (Extra STM Drain)",
                "South travel drains extra stamina! Claim Herb & move North.",
                "Deep southern cistern.",
                &herbItem2, &wraithEnemy);
            rGate.setRoomData("9. Level 2 Gate",
                "Use 'door' with Key to advance to Level 3.",
                "Gate leading to Level 3.",
                nullptr, &sentinelEnemy);
        } else {
            monkeyEnemy.resetEnemy(84, 26, "Demons rally against the Eclipse Scythe!");
            wraithEnemy.resetEnemy(98, 29, "Dark wraiths strike from the shadows!");
            golemEnemy.resetEnemy(116, 33, "Endgame golems guard the final wings!");
            sentinelEnemy.resetEnemy(136, 36, "Final stand before the endgame gate!");

            rStart.setRoomData("1. Eclipse Labyrinth Hub",
                "Lv.3 Objective: Beat the timer & maze—grab Eclipse Scythe, Relic, Key & open Final Gate!",
                "Timed endgame labyrinth. Weapons and demons stand across East, West, and North.",
                &torchItem, nullptr);
            rWest1.setRoomData("2. West Scythe Vault",
                "Claim Eclipse Scythe [Blade] & defeat Demon Monkey!",
                "Western vault holding the Eclipse Scythe and a demon.",
                &bladeItem, &monkeyEnemy);
            rWest2.setRoomData("3. West Void Dead End",
                "Dead end! Fight Shadow Wraith & turn East.",
                "Dead-end western void pocket.",
                &herbItem1, &wraithEnemy, true, false);
            rEast1.setRoomData("4. East Ash Crucible",
                "Defeat Obsidian Golem & solve the riddle ('search').",
                "Eastern crucible guarded by a demon.",
                &relicItem, &golemEnemy, false, false, "",
                "The more you take from me, the bigger I grow. What am I?", "hole");
            rEast2.setRoomData("5. East Rift (Lethal Turn)",
                "Lethal wrong turn!",
                "Tearing void chasm at the eastern edge!",
                nullptr, nullptr, false, true, "Lethal wrong turn into the East Void Rift!");
            rNorth1.setRoomData("6. North Ash Labyrinth",
                "Defeat Demon Monkey & move North.",
                "Ash-swept northern passage.",
                nullptr, &monkeyEnemy);
            rNorth2.setRoomData("7. North Crucible Sanctum",
                "Claim [Key] & Herb, defeat Golem, push North to Final Gate.",
                "Northern sanctum before the final gate.",
                &keyItem, &golemEnemy);
            rSouthVault.setRoomData("8. South Abyssal Well (Extra STM Drain)",
                "South travel drains extra stamina! Watch your clock & stamina.",
                "Southern well draining stamina fast.",
                &herbItem2, &wraithEnemy);
            rGate.setRoomData("9. Endgame Final Gate",
                "Use 'door' with Key to complete Level 3!",
                "Final gate of the 3-level campaign.",
                nullptr, &sentinelEnemy);
        }

        out << "\n=== LEVEL " << levelNumber << " ===\n";
        currentRoom->describe(out, player.getHealth(), player.getMaxHealth(), player.getStamina());
    }

    std::string step(const std::string& rawInput) {
        std::ostringstream out;
        std::string input = trimStr(rawInput);
        std::string lower = toLowerStr(input);
        lastAnimAction = "idle";

        if (lower.rfind("start ", 0) == 0) {
            std::string rest = trimStr(lower.substr(6));
            int av = 1, lv = 1;
            if (rest.find("lyra") != std::string::npos || rest.find("2") != std::string::npos) av = 2;
            if (rest.find("thorne") != std::string::npos || rest.find("3") != std::string::npos) av = 3;
            if (rest.find("level2") != std::string::npos || rest.find("lvl2") != std::string::npos) lv = 2;
            if (rest.find("level3") != std::string::npos || rest.find("lvl3") != std::string::npos) lv = 3;
            player.configureAvatar(av);
            configureLevel(lv, out);
            return out.str();
        }

        if (lower == "timeout") {
            restartCurrentLevel("Time ran out on Level 3!", out);
            return out.str();
        }

        if (worldDestroyed) {
            lastAnimAction = "apocalypse";
            out << "\n*** CAMPAIGN COMPLETE! ***\n";
            return out.str();
        }

        if (awaitingRiddle) {
            awaitingRiddle = false;
            currentRoom->solveRiddle(input, player, out);
            if (!player.isAlive()) {
                restartCurrentLevel("You fell to a trap rune!", out);
                return out.str();
            }
            currentRoom->describe(out, player.getHealth(), player.getMaxHealth(), player.getStamina());
            return out.str();
        }

        auto tryMove = [&](Room* nextRoom, const std::string& dirName, int stamCost, const std::string& animName) {
            Enemy* foe = currentRoom->getEnemy();
            if (dirName == "north" && foe && foe->isAlive()) {
                out << "\n[Blocked] " << foe->getName() << " guards the northern path! Fight first.\n";
                return;
            }
            if (dirName == "north" && !currentRoom->isRiddleSolved()) {
                out << "\n[Sealed] Solve the room riddle ('search') before moving North.\n";
                return;
            }
            if (!nextRoom) {
                out << "\n[Wall] Solid stone blocks " << dirName << ".\n";
                return;
            }
            lastAnimAction = animName;
            currentRoom = nextRoom;
            player.drainMoveStamina(stamCost, out);
            out << "\n[Move " << dirName << "] -" << stamCost << " STM (Remaining: " << player.getStamina() << ")\n";
            if (!player.isAlive()) {
                restartCurrentLevel("Exhaustion and wounds overcame you!", out);
                return;
            }
            if (currentRoom->isLethal()) {
                restartCurrentLevel(currentRoom->getLethalMsg(), out);
                return;
            }
            if (currentRoom->isDeadEnd()) {
                out << "[Dead End] You hit a dead end! You must turn back.\n";
            }
        };

        if (lower == "north") {
            tryMove(currentRoom->getNorth(), "north", 12, "run_north");
        } else if (lower == "south" || lower == "retreat") {
            tryMove(currentRoom->getSouth(), "south", 18, "run_south");
        } else if (lower == "east") {
            tryMove(currentRoom->getEast(), "east", 14, "run_east");
        } else if (lower == "west") {
            tryMove(currentRoom->getWest(), "west", 14, "run_west");
        } else if (lower == "take") {
            Item* it = currentRoom->takeRoomItem();
            if (it) {
                player.getInventory().addItem(it, out);
                lastAnimAction = (it->getName() == "Relic") ? "relic_found" : "take";
            } else {
                out << "\n[Info] Nothing left to take here.\n";
            }
        } else if (lower.rfind("use ", 0) == 0) {
            player.useItem(trimStr(input.substr(4)), out);
        } else if (lower == "rest") {
            player.gainStamina(32);
            player.heal(10, out);
            out << "\n[Rest] You catch your breath (+32 STM, +10 HP).\n";
        } else if (lower == "search") {
            if (currentRoom->hasRiddle() && !currentRoom->isRiddleSolved()) {
                awaitingRiddle = true;
                out << "\n[Riddle] " << currentRoom->getRiddlePrompt() << "\n";
            } else {
                out << "\n[Inspect] Nothing new to unlock here.\n";
            }
        } else if (lower == "slash" || lower == "attack") {
            Enemy* foe = currentRoom->getEnemy();
            if (foe && foe->isAlive()) {
                lastAnimAction = "slash";
                int dmg = player.getAttackPower(levelNumber);
                out << "\n[Attack] " << player.getName() << " strikes " << foe->getName() << " for " << dmg << " DMG!\n";
                foe->takeDamage(dmg, out);
                if (foe->isAlive()) foe->strikePlayer(player, out, false);
                else out << "\n[Defeated] " << foe->getName() << " falls!\n";
            } else {
                out << "\n[Info] No foe here.\n";
            }
        } else if (lower == "vault") {
            Enemy* foe = currentRoom->getEnemy();
            if (foe && foe->isAlive()) {
                lastAnimAction = "vault";
                if (!player.spendStamina(25)) {
                    out << "\n[Low Stamina] Need 25 STM to vault!\n";
                    foe->strikePlayer(player, out, false);
                } else {
                    int dmg = static_cast<int>(player.getAttackPower(levelNumber) * 1.45) + 8;
                    out << "\n[Vault] Heavy vault strike for " << dmg << " DMG! (-25 STM)\n";
                    foe->takeDamage(dmg, out);
                }
            } else {
                out << "\n[Info] No foe here.\n";
            }
        } else if (lower == "guard") {
            Enemy* foe = currentRoom->getEnemy();
            if (foe && foe->isAlive()) {
                lastAnimAction = "guard";
                player.gainStamina(20);
                int cDmg = player.getAttackPower(levelNumber) / 2;
                out << "\n[Guard] Riposte for " << cDmg << " DMG (+20 STM)!\n";
                foe->takeDamage(cDmg, out);
                if (foe->isAlive()) foe->strikePlayer(player, out, true);
            } else {
                out << "\n[Info] No foe here.\n";
            }
        } else if (lower == "torch") {
            Enemy* foe = currentRoom->getEnemy();
            if (foe && foe->isAlive()) {
                if (!player.getInventory().hasItem("Torch")) {
                    out << "\n[No Torch] Claim the Torch first!\n";
                } else if (!player.spendStamina(20)) {
                    out << "\n[Low Stamina] Need 20 STM for Torch Flare!\n";
                } else {
                    lastAnimAction = "torch";
                    int dmg = 28 + levelNumber * 4;
                    out << "\n[Torch Flare] Sears " << foe->getName() << " for " << dmg << " DMG! (-20 STM)\n";
                    foe->takeDamage(dmg, out);
                    if (foe->isAlive()) player.takeDamage(foe->getAttackDamage() / 2, out);
                }
            } else {
                player.useItem("Torch", out);
            }
        } else if (lower == "door") {
            if (currentRoom == &rGate) {
                if (!player.getInventory().hasItem("Key")) {
                    out << "\n[Locked] You need [Key] to open this gate.\n";
                } else if (levelNumber == 1) {
                    out << "\n[Level 1 Cleared!] Advancing to Level 2!\n";
                    configureLevel(2, out);
                    lastAnimAction = "level_up";
                    return out.str();
                } else if (levelNumber == 2) {
                    out << "\n[Level 2 Cleared!] Advancing to Level 3!\n";
                    configureLevel(3, out);
                    lastAnimAction = "twist";
                    return out.str();
                } else {
                    worldDestroyed = true;
                    lastAnimAction = "apocalypse";
                    out << "\n*** VICTORY! ALL 3 LEVELS CONQUERED! ***\n";
                    return out.str();
                }
            } else {
                out << "\n[Info] Reach the Gate chamber to use 'door'.\n";
            }
        } else {
            out << "\n[?] Unknown command '" << input << "'.\n";
        }

        if (!player.isAlive()) {
            restartCurrentLevel("You were defeated!", out);
            return out.str();
        }

        currentRoom->describe(out, player.getHealth(), player.getMaxHealth(), player.getStamina());
        return out.str();
    }

    std::string getRoomId() const { return currentRoom->getId(); }
    int getHp() const { return player.getHealth(); }
    int getMaxHp() const { return player.getMaxHealth(); }
    int getStamina() const { return player.getStamina(); }
    int getMaxStamina() const { return player.getMaxStamina(); }
    int getHasTorch() const { return player.getInventory().hasItem("Torch") ? 1 : 0; }
    int getHasBlade() const { return player.getInventory().hasItem("Blade") ? 1 : 0; }
    int getHasHerb() const { return player.getInventory().hasItem("Herb") ? 1 : 0; }
    int getHasKey() const { return player.getInventory().hasItem("Key") ? 1 : 0; }
    int getHasRelic() const { return player.getInventory().hasItem("Relic") ? 1 : 0; }
    int getEnemyHp() const {
        Enemy* foe = currentRoom->getEnemy();
        return (foe && foe->isAlive()) ? foe->getHealth() : 0;
    }
    int getEnemyMaxHp() const {
        Enemy* foe = currentRoom->getEnemy();
        return foe ? foe->getMaxHealth() : 0;
    }
    const char* getEnemyNameCstr() const {
        static std::string s;
        Enemy* foe = currentRoom->getEnemy();
        s = (foe && foe->isAlive()) ? foe->getName() : "";
        return s.c_str();
    }
    const char* getEnemyKindCstr() const {
        static std::string s;
        Enemy* foe = currentRoom->getEnemy();
        s = (foe && foe->isAlive()) ? foe->getKind() : "";
        return s.c_str();
    }
    const char* getPromptModeCstr() const {
        static std::string s;
        s = awaitingRiddle ? "riddle_gate" : "";
        return s.c_str();
    }
    const char* getLastAnimCstr() const { return lastAnimAction.c_str(); }
    int getLevelNumber() const { return levelNumber; }
    int getWorldDestroyedFlag() const { return worldDestroyed ? 1 : 0; }
    int getAvatarId() const { return player.getAvatarId(); }
    int getDefeatsCount() const { return defeatsCount; }
};

static GameEngine g_engine;

EXPORT const char* run_command(const char* input) {
    static std::string response;
    response = g_engine.step(input ? input : "");
    return response.c_str();
}
EXPORT const char* current_room() { static std::string s; s = g_engine.getRoomId(); return s.c_str(); }
EXPORT int current_hp() { return g_engine.getHp(); }
EXPORT int max_hp() { return g_engine.getMaxHp(); }
EXPORT int current_stamina() { return g_engine.getStamina(); }
EXPORT int max_stamina() { return g_engine.getMaxStamina(); }
EXPORT int has_torch_flag() { return g_engine.getHasTorch(); }
EXPORT int has_blade_flag() { return g_engine.getHasBlade(); }
EXPORT int has_herb_flag() { return g_engine.getHasHerb(); }
EXPORT int has_key_flag() { return g_engine.getHasKey(); }
EXPORT int has_relic_flag() { return g_engine.getHasRelic(); }
EXPORT int enemy_hp() { return g_engine.getEnemyHp(); }
EXPORT int enemy_max_hp() { return g_engine.getEnemyMaxHp(); }
EXPORT const char* enemy_name() { return g_engine.getEnemyNameCstr(); }
EXPORT const char* enemy_kind() { return g_engine.getEnemyKindCstr(); }
EXPORT const char* prompt_mode() { return g_engine.getPromptModeCstr(); }
EXPORT const char* last_anim() { return g_engine.getLastAnimCstr(); }
EXPORT int current_level() { return g_engine.getLevelNumber(); }
EXPORT int is_world_destroyed() { return g_engine.getWorldDestroyedFlag(); }
EXPORT int is_escaped_flag() { return g_engine.getWorldDestroyedFlag(); }
EXPORT int current_avatar_id() { return g_engine.getAvatarId(); }
EXPORT int defeats_count() { return g_engine.getDefeatsCount(); }
