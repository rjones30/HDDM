import hddm_s

for rec in hddm_s.istream("http://nod25.phys.uconn.edu:2880/Gluex/simulation" +
                          "/simsamples/particle_gun-v5.2.0/particle_gun001_019.hddm"):
   for pe in rec.getPhysicsEvents():
      print(f"found run {pe.runNo}, event {pe.eventNo}")

for rec in hddm_s.istream("https://nod25.phys.uconn.edu:2843/Gluex/simulation" +
                          "/simsamples/particle_gun-v5.2.0/particle_gun001_019.hddm"):
   for pe in rec.getPhysicsEvents():
      print(f"then found run {pe.runNo}, event {pe.eventNo}")
