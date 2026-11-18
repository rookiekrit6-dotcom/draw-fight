# ตัวละครสำเร็จรูป ชุดแรก (8 ตัว)

ข้อมูลจริงอยู่ใน `data/DT_Characters.csv` (import เป็น DataTable แบบ `DFCharacterRow`)

ทุกตัวมี 3 ช่องสกิล: ช่อง 1 ล็อกเป็นสกิลประจำตัว · ช่อง 2-3 เปลี่ยนเป็นการ์ดสกิลที่เก็บได้ · ใส่อาชีพลับเพิ่มท่าไม้ตายปุ่ม R

| ตัว | ธาตุ | อาวุธ | สกิล | ติดตัว | เลือด/ตี/เร็ว/ขนาด | ปลดล็อก |
|---|---|---|---|---|---|---|
| **เพลิงกาฬ** (Blaze) | ไฟ | ดาบ | คลื่นดาบ, อุกกาบาต, คลื่นดาบ | สมาธิ | 1.0/1.1/1.0/1.0 | ฟรี |
| **ธาราเงียบ** (Tide) | น้ำ | คทาเวท | ฟื้นพลัง, เกราะธาตุ, อุกกาบาต | ฟื้นตัว | 1.05/0.85/1.0/1.0 | ฟรี |
| **ภูผา** (Granite) | ดิน | ค้อน | ทุบปฐพี, แผ่นดินไหว, ทุบปฐพี | หัวแข็ง | 1.35/1.1/0.85/1.15 | ฟรี |
| **วายุพเนจร** (Gale) | ลม | หอก | พุ่งทะลวง, พายุหมุน, พุ่งทะลวง | หัวจิ๋วหลบไว | 0.9/1.0/1.2/0.95 | 300 ทอง |
| **อัสนี** (Volt) | แสง | ปืนรัว | อัสนีบาต, เร่งความเร็ว, ฝนธนู | สมาธิ | 0.9/1.05/1.05/1.0 | 300 ทอง |
| **เงาราตรี** (Nox) | มืด | มีดสั้นนักฆ่า | ก้าวเงา, ตัดมิติ, ก้าวเงา | หัวจิ๋วหลบไว | 0.8/1.2/1.25/0.9 | 600 ทอง |
| **ไททันเหล็ก** (Titan) | จักรกล | โล่ | พุ่งกระแทก, สวนกลับ, พุ่งกระแทก | หัวแข็ง | 1.5/0.9/0.8/1.25 | 600 ทอง |
| **หมึกน้อย** (Inkling) | พลังจิต | สเปรย์ | หมึกหก, พลิกหน้ากระดาษ, หมึกหก | สมาธิ | 0.95/1.0/1.05/0.85 | 900 ทอง |

## หน้าตา (บรีฟสำหรับปั้นโมเดล)

### เพลิงกาฬ — นักดาบเปลวเพลิง
- สี: `#e5484d`
- หน้าตา: ชายหนุ่มผมแดงชี้ ผ้าพันคอยาวสะบัดเป็นเปลวไฟ เกราะครึ่งตัวสีดำขอบทอง ดาบยาวใบมีดเรืองแดงเหมือนเหล็กเพิ่งออกจากเตา
- Prompt อังกฤษ: Young swordsman with spiky red hair, long scarf that trails like flame, black half-armor with gold trim, a longsword glowing like fresh-forged steel, stylized 3D game character, full body, T-pose, clean topology
- คำพูด: "ไฟของข้าไม่เคยดับ จนกว่าศัตรูจะเป็นเถ้า"

### ธาราเงียบ — นักบวชแห่งกระแสน้ำ
- สี: `#2b7de9`
- หน้าตา: หญิงสาวผมยาวสีฟ้าน้ำทะเล ชุดคลุมขาวลายคลื่น คทาปลายเป็นหยดน้ำลอยอยู่กลางวงแหวน มีละอองน้ำวนรอบตัวตลอดเวลา
- Prompt อังกฤษ: Young woman with long sea-blue hair, white robe with wave patterns, a staff topped by a floating water droplet inside a ring, mist always circling her, stylized 3D game character, full body, T-pose, clean topology
- คำพูด: "น้ำนิ่งไหลลึก อย่าคิดว่าข้าอ่อนแอ"

### ภูผา — ยักษ์ศิลา
- สี: `#8a5a2b`
- หน้าตา: ชายร่างใหญ่ไหล่กว้าง ผิวมีรอยแตกเหมือนหิน ไหล่เป็นแผ่นหินแทนเกราะ ค้อนใหญ่หัวเป็นก้อนหินมีรูนเรืองส้ม
- Prompt อังกฤษ: Huge broad-shouldered man with cracked stone-like skin, rock slabs for shoulder armor, a giant hammer whose head is a boulder with glowing orange runes, stylized 3D game character, full body, T-pose, clean topology
- คำพูด: "ภูเขาไม่เคยถอย"

### วายุพเนจร — นักหอกสายลม
- สี: `#5cc8e0`
- หน้าตา: หนุ่มร่างเพรียว ผ้าคาดตาสีเขียวมรกตปลิว เสื้อคลุมแขนกุดสีขาวเขียว หอกยาวมีขนนกผูกที่ด้าม มีกระแสลมเส้นบางๆ วนรอบขา
- Prompt อังกฤษ: Lean young lancer, emerald headband streaming in the wind, sleeveless white-green coat, long spear with feathers tied to the shaft, thin wind streaks around his legs, stylized 3D game character, full body, T-pose, clean topology
- คำพูด: "จับลมได้ ค่อยมาจับข้า"

### อัสนี — มือปืนสายฟ้า
- สี: `#f5c518`
- หน้าตา: หญิงสาวผมสั้นสีทองชี้ฟู แว่นกันลมบนหัว แจ็กเก็ตสีกรมท่ามีลายวงจรเรืองเหลือง ปืนยาวลำกล้องคู่มีประกายไฟฟ้าแลบ
- Prompt อังกฤษ: Short-haired girl with spiky gold hair, goggles on her head, navy jacket with glowing yellow circuit lines, twin-barrel rifle crackling with sparks, stylized 3D game character, full body, T-pose, clean topology
- คำพูด: "เร็วกว่าฟ้าผ่า แรงกว่าฟ้าร้อง"

### เงาราตรี — นักฆ่าแห่งเงา
- สี: `#7b4bd6`
- หน้าตา: นักฆ่าใส่ฮู้ดสีดำม่วง ผ้าปิดหน้าครึ่งล่าง ดวงตาเรืองม่วง มีดสั้นคู่ใบโค้ง ควันดำไหลออกจากชายเสื้อ
- Prompt อังกฤษ: Assassin in a black-purple hood, lower face masked, glowing violet eyes, twin curved daggers, black smoke dripping from the coat hem, stylized 3D game character, full body, T-pose, clean topology
- คำพูด: "เจ้าจะไม่เห็นข้า จนกว่าจะสายเกินไป"

### ไททันเหล็ก — อัศวินจักรกล
- สี: `#9aa4b2`
- หน้าตา: หุ่นอัศวินตัวใหญ่เกราะเหล็กหนา มีท่อไอน้ำที่หลัง ดวงตาเป็นเลนส์สีฟ้า โล่ทรงหอคอยมีเฟืองหมุนกลางโล่
- Prompt อังกฤษ: Large mech knight in thick steel plating, steam pipes on the back, blue lens eyes, a tower shield with a spinning gear at its center, stylized 3D game character, full body, T-pose, clean topology
- คำพูด: "ผ่านโล่นี้ไปให้ได้ก่อน"

### หมึกน้อย — จอมเวทน้ำหมึก
- สี: `#2c2c3a`
- หน้าตา: ตัวละครที่หลุดออกมาจากสมุดวาดเขียน ตัวเล็ก เส้นขอบเป็นลายเส้นดินสอ ผมเป็นหยดหมึกสีดำ ถือขวดสเปรย์หมึก มีหน้ากระดาษปลิวรอบตัว
- Prompt อังกฤษ: A small hero who stepped out of a sketchbook, pencil-line outlines, hair made of black ink drops, holding an ink spray bottle, loose pages fluttering around, stylized 3D game character, full body, T-pose, clean topology
- คำพูด: "วาดเสร็จเมื่อไหร่ เจ้าก็จบเมื่อนั้น"

## เศรษฐกิจ
- เริ่มเกมได้ 300 ทอง · ชนะ +120 · แพ้ +30
- ไม่มีร้านสุ่มการ์ดแล้ว: ชนะ 1 ครั้งได้การ์ดสกิลฟรี 1 ใบ · ทองใช้หมุนออร่า (ครั้งละ 10) และปลดล็อกตัวละคร · ดู AURA_CLASSES.md
- ตัวละครฟรี 3 ตัว (เพลิงกาฬ ธาราเงียบ ภูผา) · 300 ทอง 2 ตัว · 600 ทอง 2 ตัว · หมึกน้อย 900 ทอง (มาสคอตของเกม)
