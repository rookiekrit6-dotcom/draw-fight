import csv
B=[
("thrust",2,"Dash","Speed trail ribbon behind the body, spear tip spark cone, pierce flash on each enemy passed","Short forward dash montage, spear held level"),
("slam",1,"Ground AOE","Radial crack decal + dust ring, rock chunks (mesh particles) launched, shockwave distortion ring; mid-air version: falling streak then bigger crater","Overhead raise then smash; air variant = dive"),
("throw",3,"Projectiles x3","Three spinning weapon copies (use the player's own weapon mesh) with ribbon trails, impact sparks","Quick sidearm throw"),
("spin",2,"Self AOE 1.2s","Ring of motion-blur slashes around the body, wind swirl particles, hit sparks","Spin loop montage 1.2s"),
("wave",1,"Line projectile","Crescent blade mesh with emissive edge + bloom, ground scorch line, pierces every enemy in line","Wide horizontal slash"),
("volley",2,"5 arcing projectiles","Arrows with glow trails arcing up, impact puffs at 5 landing spots, warning dots on ground","Bow draw aimed upward"),
("meteor",1,"Delayed AOE","Magic circle telegraph on target (radius readable), meteor falling from sky with fire trail, big blast + crater + camera shake","Staff raised to sky, then point down"),
("shadow",1,"Teleport strike","Body dissolves into black smoke, reappears behind target with purple afterimage, one bright stab flash","Vanish, appear, stab"),
("lash",2,"Pull line","Whip ribbon with glowing tip reaching full range, drag streak on the ground as the enemy is pulled","Long whip crack"),
("charge",2,"Shield dash","Energy shield bubble in front, ground sparks, knockback burst on contact","Hunched shield charge loop"),
("blast",2,"Self AOE","Expanding fireball sphere around the hero, heat distortion, debris and smoke column","Arms out burst pose"),
("heal",3,"Self buff","Green rising motes + soft ring at feet, brief body glow","Short raise-hand pose"),
("rage",2,"Self buff 5s","Red aura flames on body for 5s, veins of light, red screen-edge pulse at start","Roar pose"),
("haste",3,"Self buff 5s","Wind streak lines around legs, afterimages while running","Quick crouch"),
("barrier",2,"Self buff 3s","Hex shield dome tinted by hero element, ripple where hits land","Brace pose"),
("quake",1,"Wide ground AOE","Arena-wide cracks spreading from the hero, rock pillars burst up under grounded enemies, heavy shake","Stomp"),
("vortex",1,"Pull AOE","Black-purple swirl disc on ground, debris spiraling inward, enemies dragged","Both hands pull inward"),
("counter",2,"Stance","Glowing outline on hero during stance; on trigger: time-slow flash, white slash cross","Guard stance, then counter slash"),
("leap",2,"Gap closer","Jump arc trail, landing impact ring + dust","Leap and smash"),
("sun",1,"Arena-wide 3s","Small sun in the sky, god rays, whole-arena warm light (drive a light from the skill), enemies ignite","Both arms to the sky"),
("thunder",1,"3 strikes","Warning line on ground before each strike, lightning bolt (beam + branches), electric sparks, white flash","Point at target"),
("rift",1,"Arena line","Horizontal dimension tear across the arena (glowing slit with distortion), jumpable","Horizontal hand chop"),
("miasma",2,"Lingering cloud","Long green-purple smoke cloud, bubbling decals on the ground, 4.5s","Breath forward"),
("tornado",1,"Moving AOE","Tornado mesh with scrolling noise texture, debris orbiting, lifts enemies","Push forward"),
("inkspill",3,"Ground pool","Glossy black ink puddle decal with drips, slow splash on step","Flick forward"),
("eraser",2,"Debuff hit","Pink eraser crumbs burst, the target's buff icons visibly erased","Rubbing motion"),
("ruler",2,"Screen-wide line","Giant ruler mesh slapping across the lane, motion streak","Wide swing"),
("scribble",2,"6-hit combo","Scribble line squiggles in the air for each hit, last hit launches with a big doodle star","Six quick strokes"),
("pin",2,"Stun","Giant thumbtack drops from above onto target, stars circling","Point down"),
("pageflip",2,"Swap places","Paper page sweeping across screen, both fighters swap behind it","Page turn gesture"),
("clip",2,"Grab and slam","Paper clip meshes snapping onto enemies, lift, then slam with ring","Clamp then slam"),
("pause",1,"Time stop","Desaturate everything inside a sphere, clock-hand ring, frozen particles","Palm out"),
("sig",2,"Built from parts","Compose from the Move + Attack + Effect parts in DT_SignatureParts; each part gets its own small VFX module","Depends on parts"),
]
rows=list(csv.DictReader(open('data/DT_Skills.csv',encoding='utf-8-sig')))
names={r['Name']:r for r in rows}
assert set(names)==set(b[0] for b in B),set(names)^set(b[0] for b in B)
with open('data/skill_vfx_briefs.csv','w',encoding='utf-8-sig',newline='') as f:
    w=csv.writer(f,lineterminator='\r\n');w.writerow(['Name','NameTH','NameEN','Rarity','Priority','Type','VFX','Animation','Status'])
    for k,p,t,v,a in sorted(B,key=lambda b:(b[1],b[0])):
        r=names[k];w.writerow([k,r['NameTH'],r['NameEN'],r['Rarity'],p,t,v,a,'todo'])
print(sum(1 for b in B if b[1]==1),'priority-1 skills:',[b[0] for b in B if b[1]==1])
