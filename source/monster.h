#pragma pack(push,1)

//Parts info
typedef struct
{
  u16 unknown1;		//0x0
  u32 fixed;		//0x2: F8 99 D0 00
  u8 stagger_count;	//0x6
  u8 break_count;	//0x7
  u16 stagger_hp;	//0x8
  s16 break_hp;		//0xA	
} Part;

//Monster info (incomplete)
typedef struct
{
  u8 static1;       //0x0: potential monster id for big (but special has same byte), but random for small
  u16 static2;      //different for each big monster(except special, and changes when not first monster), but for small monsters is B1 4F or B2 4F
  u8 fixed1;        //fixed at 1
  u32 unused1[2];   //unused
  u8 fixed2;        //fixed at 22
  u8 location_flag; //4C = in current map, 44 = not in current map
  u8 fixed3;        //fixed at 1
  u8 vary1;         //mosly 00 for large but can be different (Lv10 Hellblade),varies for small
  u8 is_despawned;  //7 = despawned, F = visible
  u8 fixed4;		//fixed at 8
  u16 unused2;		//unused
  u32 pointer1[2];  //can be null; always null at base camp, both always different
  u8 dynamic1[3];   //changes all the time
  u8 unknown1;      //40 for big monsters (sometimes not true), dynamic for small
  u8 ignore1[0xEC];
  u32 pair1;        //0x10C: same as pair2; random value for big, last 3 are FF for small; for small first byte can be dynamic
  u32 test2;        //0x110: random value for big, FFFFFFFF or 0 for small
  u8 ignore2[0xA8];
  u32 pair2;        //0x1BC: same as pair1
  u8 ignore3[0x68];
  u8 static3;       //0x228: potential monster id for big (but special has same byte)
  u8 ignore4[0x9A3];
  u8 unknown2;      //0xBCC: single digit numbers, seems to be 1 for small but can vary for large
  u8 ignore5[0x84B];
  u32 hp;
  u32 max_hp;
  u8 ignore6[0x3E];
  Part parts[8];
} Monster;

#pragma pack(pop)