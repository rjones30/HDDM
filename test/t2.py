import hddm_s

for rec in hddm_s.istream("root://nod25.phys.uconn.edu/Gluex/simulation" +
                          "/simsamples/particle_gun-v5.2.0/particle_gun001_019.hddm"):
   for pe in rec.getPhysicsEvents():
      print(f"found run {pe.runNo}, event {pe.eventNo}")
