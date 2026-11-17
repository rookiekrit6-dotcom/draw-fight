const D=require('./raw.json'),A=require('./data/all_game_data.json');
// original
const gaFit0=(k,cl,el)=>{let m=1;if(D.CLASS[cl]&&D.CLASS[cl].skill===k)m=4;else for(const g of D.GA_GRP)if(g[0].indexOf(cl)>=0&&g[1].indexOf(k)>=0)m=2;if((D.GA_EL[el]||[]).indexOf(k)>=0)m*=3;return m;};
// port (same as DFGacha.cpp, reading only the CSV-derived rows)
const grpOf=cl=>D.GA_GRP[0][0].includes(cl)?'Melee':D.GA_GRP[1][0].includes(cl)?'Ranged':'Any';
const has=(l,k)=>l.split(';').filter(Boolean).includes(k);
const gaFit1=(r,cl,el)=>{let m=1;if(has(r.SignatureOfClass,cl))m=4;else if(r.GachaGroup!=='Any'&&r.GachaGroup===grpOf(cl))m=2;if(has(r.FavoredElements,el))m*=3;return m;};
let bad=0,n=0;for(const cl of Object.keys(D.CLASS))for(const el of Object.keys(D.ELEM))for(const r of A.skills){if(r.Name==='sig')continue;n++;if(gaFit0(r.Name,cl,el)!==gaFit1(r,cl,el)){bad++;if(bad<6)console.log('diff',r.Name,cl,el,gaFit0(r.Name,cl,el),gaFit1(r,cl,el));}}
console.log('fit checks',n,'mismatches',bad);
// csv parse check: every CSV has same column count on each row
const fs=require('fs');for(const f of fs.readdirSync('data').filter(f=>f.endsWith('.csv'))){const t=fs.readFileSync('data/'+f,'utf8').replace(/^﻿/,'');const rows=[];let row=[],cur='',q=false;for(let i=0;i<t.length;i++){const c=t[i];if(q){if(c==='"'&&t[i+1]==='"'){cur+='"';i++;}else if(c==='"')q=false;else cur+=c;}else if(c==='"')q=true;else if(c===','){row.push(cur);cur='';}else if(c==='\r'){}else if(c==='\n'){row.push(cur);rows.push(row);row=[];cur='';}else cur+=c;}
 const w=rows[0].length,badr=rows.filter(r=>r.length!==w).length,names=new Set(rows.slice(1).map(r=>r[0]));console.log(f,'rows',rows.length-1,'cols',w,'bad rows',badr,'unique names',names.size===rows.length-1);}
