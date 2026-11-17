const fs=require('fs');const D=require('./raw.json');const EN=D.EN;
const en=t=>t&&EN[t]?EN[t]:'';
const q=v=>{if(v==null)return '""';if(typeof v==='number')return String(v);v=String(v);return '"'+v.replace(/"/g,'""')+'"';};
function csv(file,cols,rows){const out=[cols.join(',')].concat(rows.map(r=>cols.map(c=>q(r[c])).join(',')));fs.writeFileSync('data/'+file,'﻿'+out.join('\r\n')+'\r\n');return rows.length;}
const grp=k=>D.GA_GRP[0][1].includes(k)?'Melee':D.GA_GRP[1][1].includes(k)?'Ranged':'Any';
const elOf=k=>Object.keys(D.GA_EL).filter(e=>D.GA_EL[e].includes(k)).join(';');
const clsOf=k=>Object.keys(D.CLASS).filter(c=>D.CLASS[c].skill===k).join(';');
const RN=['','Common','Rare','Legendary'];
const skills=Object.entries(D.SKILL).map(([k,v])=>({Name:k,NameTH:v.n,NameEN:en(v.n),DescTH:v.d,DescEN:en(v.d),Cooldown:v.cd,Rarity:RN[D.RAR[k]||0]||'Signature',WeaponHintTH:v.w||'',SignatureOfClass:clsOf(k),GachaGroup:grp(k),FavoredElements:elOf(k),IconShape:D.SKICON[k]??-1}));
const n1=csv('DT_Skills.csv',['Name','NameTH','NameEN','DescTH','DescEN','Cooldown','Rarity','WeaponHintTH','SignatureOfClass','GachaGroup','FavoredElements','IconShape'],skills);
const els=Object.entries(D.ELEM).map(([k,v])=>({Name:k,NameTH:v.n,NameEN:en(v.n),SuffixTH:v.sfx||'',Color:v.c||'',TextColor:v.tc||'',EffectTH:v.d||'',EffectEN:en(v.d),SoundFreq:v.f||0,Beats:(D.BEATS[k]||[]).join(';')}));
const n2=csv('DT_Elements.csv',['Name','NameTH','NameEN','SuffixTH','Color','TextColor','EffectTH','EffectEN','SoundFreq','Beats'],els);
const cls=Object.entries(D.CLASS).map(([k,v])=>({Name:k,NameTH:v.n,NameEN:en(v.n),DescTH:v.d,DescEN:en(v.d),SignatureSkill:v.skill||'',RangedDistance2D:v.ranged||0,DamageMult:v.dmg??1,ReachMult:v.reach??1,CooldownMult:v.cd??1,SigMoves:(D.SIGCLS[k]||[]).join(';'),DrawnShapeTypes:Object.keys(D.WT2C).filter(w=>D.WT2C[w]===k).join(';')}));
const n3=csv('DT_WeaponClasses.csv',['Name','NameTH','NameEN','DescTH','DescEN','SignatureSkill','RangedDistance2D','DamageMult','ReachMult','CooldownMult','SigMoves','DrawnShapeTypes'],cls);
const perks=Object.entries(D.PERK).map(([k,v])=>({Name:k,NameTH:v.n,NameEN:en(v.n),DescTH:v.d,DescEN:en(v.d),CostTH:v.c||'',CostEN:en(v.c)}));
const n4=csv('DT_Perks.csv',['Name','NameTH','NameEN','DescTH','DescEN','CostTH','CostEN'],perks);
const pas=Object.entries(D.PASSIVE).map(([k,v])=>({Name:k,NameTH:v.n,NameEN:en(v.n),DescTH:v.d,DescEN:en(v.d)}));
const n5=csv('DT_Passives.csv',['Name','NameTH','NameEN','DescTH','DescEN'],pas);
const kinds=Object.entries(D.KIND).map(([k,v])=>({Name:k,NameTH:v.n,HpMult:v.hp??1,DamageMult:v.dmg??1,SizeMult:v.sz??1,SpeedMult:v.sp??1,Heavy:v.heavy?'True':'False'}));
const n6=csv('DT_EnemyKinds.csv',['Name','NameTH','HpMult','DamageMult','SizeMult','SpeedMult','Heavy'],kinds);
const boss=D.HDBOSS.map((b,i)=>({Name:'boss'+(i+1),NameTH:b[0],NameEN:en(b[0]),Kind:b[1],WeaponShape:b[2],SizeMult:b[3],Pattern:b[4]}));
const n7=csv('DT_Bosses.csv',['Name','NameTH','NameEN','Kind','WeaponShape','SizeMult','Pattern'],boss);
const sig=[];for(const [t,o] of [['Move',D.SIGM],['Attack',D.SIGA],['Effect',D.SIGE]])for(const k in o)sig.push({Name:t+'_'+k,Part:t,Key:k,NameTH:o[k][0],NameEN:en(o[k][0]),Weight:o[k][1]});
const n8=csv('DT_SignatureParts.csv',['Name','Part','Key','NameTH','NameEN','Weight'],sig);
const kw=D.SIGKW.map((r,i)=>({Name:'kw'+(i+1),Keywords:r[0].join(';'),Move:r[1],Attack:r[2],Effect:r[3],Anim:r[4],SkillNameTH:r[5],SkillNameEN:en(r[5])}));
const n9=csv('DT_SignatureKeywords.csv',['Name','Keywords','Move','Attack','Effect','Anim','SkillNameTH','SkillNameEN'],kw);
const gacha={costSingle:D.GA.cost,costTen:D.GA.cost*9,pityPulls:30,pityGuarantees:'Legendary',tenPullGuarantee:'at least one Rare or better on the 10th pull if none yet',duplicateRefund:{Common:D.GA.refund[1],Rare:D.GA.refund[2],Legendary:D.GA.refund[3]},maxSkillLevel:3,baseWeightByRarity:{Common:60,Rare:30,Legendary:10},
 fitMultipliers:{signatureSkillOfYourWeaponClass:4,sameMeleeOrRangedGroup:2,favoredByYourElement:3,note:'element multiplier stacks with the class/group one (multiply)'},groups:{Melee:{classes:D.GA_GRP[0][0],skills:D.GA_GRP[0][1]},Ranged:{classes:D.GA_GRP[1][0],skills:D.GA_GRP[1][1]}},elementFavors:D.GA_EL,
 signatureSkillPower:'mult = max(0.8, 2.7 - 0.22 * (moveWeight + attackWeight + effectWeight))',comboMoves:D.MOVES};
fs.writeFileSync('data/gacha_and_rules.json',JSON.stringify(gacha,null,1));
const all={skills,elements:els,weaponClasses:cls,perks,passives:pas,enemyKinds:kinds,bosses:boss,signatureParts:sig,signatureKeywords:kw,gacha};fs.writeFileSync('data/all_game_data.json',JSON.stringify(all,null,1));
console.log({skills:n1,elements:n2,classes:n3,perks:n4,passives:n5,kinds:n6,bosses:n7,sigParts:n8,keywords:n9});
console.log('missing EN names:',skills.filter(x=>!x.NameEN).map(x=>x.NameTH),cls.filter(x=>!x.NameEN).map(x=>x.NameTH),els.filter(x=>!x.NameEN).map(x=>x.NameTH),perks.filter(x=>!x.NameEN).map(x=>x.NameTH));
