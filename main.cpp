#include <mysqlx/xdevapi.h>
#include <iostream>
#include <vector>
using namespace mysqlx;
using namespace std;

Session sess("localhost", 33060, "root", "");
Schema db = sess.getSchema("snpmb");
RowResult res;
SqlResult sqlRes;
DocResult docs;
Row row;
Value val;

bool isExist(std::string table, std::string value, std::string where, auto param){
    Table tab = db.getTable(table);

    res = tab.select(value).where(where + " = :param").bind("param", param).execute();
    bool result = res.count() > 0;
    if(!result){
        return false;
    }
    
    return true;
}

template <typename T> T data(std::string table, std::string value, std::string where, auto param){
    Table tab = db.getTable(table);
    
    RowResult res = tab.select(value).where(where + " = :param").bind("param", param).execute();

    Row row = *res.begin();

    return row[0].get<T>();
}

bool isNull(std::string table, std::string value, std::string where, auto param){
    Table tab = db.getTable(table);
    res = tab.select(value).where(where + " = :param").bind("param", param).execute();
    row = res.fetchOne();
    val = row[0];

    if(val.isNull()){
        return true;
    }

    return false;
}

int login(){
    int option, userId;
    std::string password, nisn, email, tanggalLahir, temp;
    bool exist;

    Table tab = db.getTable("akun_user");
    
    loginRegister:
    cout << "\n=== Login or Register ===\n";
    cout << "1. Login\n" << "2. Register\n" << "Enter option: ";
    cin >> option;
    
    switch (option)
    {
    case 1:
        login:
        cout << "\n=== LOGIN ===\n";
        cout << "Email: ";
        cin >> email;
        cout << "Password: ";
        cin >> password;

        exist = isExist("akun_user", "email", "email", email);
        if(!exist){
            cout << "Your email is not registered!\n";
            goto loginRegister;
        }

        temp = data<std::string>("akun_user", "password", "email", email);
        if(password != temp){
            cout << "Wrong password!\n\n";
            goto login;
        }

        break;
    case 2:
        reg:
        cout << "\n=== REGISTER ===\n";
        cout << "Email: ";
        cin >> email;
        cout << "NISN: "; // harus diganti NISN bang to make sure that the user who register is a student
        cin >> nisn;
        cout << "Create password: ";
        cin >> password;
        cout << "Tanggal lahir [YYYY-MM-DD]: "; // this also have to replace with NPSN
        cin >> tanggalLahir;

        exist = isExist("akun_user", "email", "email", email);
        if(exist){
            cout << "Email is already registered!\n";
            goto loginRegister;
        }

        exist = isExist("biodata", "nisn", "nisn", nisn);
        if(!exist){
            cout << "NISN is not found\n";
            goto reg;
        }

        tab.insert("email", "password", "tanggal_lahir")
            .values(email, password, tanggalLahir)
            .execute();
    
        userId = data<int>("akun_user", "id_user", "email", email);

        tab = db.getTable("biodata");
        tab.update()
            .set("id_user", userId)
            .where("nisn = :param")
            .bind("param", nisn)
            .execute();

        cout << "Register account successfully!\n";
        break;
    default:
        cout << "Invalid input! Please enter (1-2)\n";
        goto login;
        break;
    }

    userId = data<int>("akun_user", "id_user", "email", email);

    return userId;
}

void inputJurusan(std::string type, int userId, int biodataId){
    vector<int> jurusanId;
    std::string jurusan;
    int ptnId, jumlahJurusan, constraint;
    char confirm;
    
    if(type == "SNBP"){
        constraint = 2;
    } else if(type == "SNBT"){
        constraint = 4;
    }

        inputJurusan:
        cout << "Masukkan jumlah jurusan (1-" + to_string(constraint) + "): ";
        cin >> jumlahJurusan;

    if(jumlahJurusan > constraint || jumlahJurusan < 1){
            cout << "Masukkan jumlah jurusan dengan benar!\n";
            goto inputJurusan;
        }

    for(int i = 0; i < jumlahJurusan; i++){
        Table tab = db.getTable("ptn");
        RowResult res = tab.select("nama_ptn").execute();

        cout << "Daftar PTN\n";
        int index = 1;
        for(Row row : res.fetchAll()){
            cout << index << ". " << row.get(0).get<std::string>() << endl;
            index++;
        }

        cout << "PTN Pilihan " << i + 1 << ": ";
        cin >> ptnId;
                    // sql query
        tab = db.getTable("program_studi");
        res = tab.select("nama_prodi").where("id_ptn = :param").bind("param" ,ptnId).execute();
        
        cout << "\nDaftar Prodi\n";
        for(Row row : res.fetchAll()){
            cout << row.get(0).get<std::string>() << endl;
        }
        cin.ignore();
        inputProdi:
        cout << "Program Studi Pilihan " << i + 1 << " [nama prodi]: ";
        getline(cin, jurusan);

        if(!isExist("program_studi", "nama_prodi", "nama_prodi", jurusan)){
            cout << "Masukkan nama prodi dengan benar!\n";
            goto inputProdi;
        }
        std::string temp = "id_ptn = " + to_string(ptnId) + " && nama_prodi";
        int Id = data<int>("program_studi", "id_prodi", temp, jurusan);
        jurusanId.push_back(Id);
    }

    Table tab = db.getTable("pendaftaran_snbp");

    if(type == "SNBT"){
        tab = db.getTable("pendaftaran_snbt");
    }

    if(jumlahJurusan == 1){
        tab.insert("id_user", "id_biodata", "id_prodi1")
        .values(userId, biodataId, jurusanId.at(0))
        .execute();
    } else if (jumlahJurusan == 2){
        tab.insert("id_user", "id_biodata", "id_prodi1", "id_prodi2")
        .values(userId, biodataId, jurusanId.at(0), jurusanId.at(1))
        .execute();
    }

    cout << "Berhasil daftar " + type + "!\n";

    jurusanId.clear();
}

void soalUtbk(int userId){
    double scorePU = 0;
    char jawaban;

    cout << "\n=== UTBK ===\n";
    cout << "=== Penalaran Umum ===\n";
    cout << "1. Jika p maka q. Andi melakukan p. Apa kesimpulan yang valid?\n";
    cout << "a. Andi makan\n";
    cout << "b. Andi minum\n";
    cout << "c. Andi makan dan minum\n";
    cout << "d. Andi q\n";
    cout << "Masukkan jawaban: ";
     cin >> jawaban;
      if(jawaban == 'd') scorePU += 100/3;
    cout << "2. Jika p maka q. Andi melakukan p. Apa kesimpulan yang valid?\n";
    cout << "a. Andi makan\n";
    cout << "b. Andi q\n";
    cout << "c. Andi minum\n";
    cout << "d. Andi makan dan minum\n";
    cout << "Masukkan jawaban: ";
     cin >> jawaban;
      if(jawaban == 'd') scorePU += 100/3;
    cout << "3. Jika p maka q. Andi melakukan p. Apa kesimpulan yang valid?\n";
    cout << "a. Andi q\n";
    cout << "b. Andi makan\n";
    cout << "c. Andi minum\n";
    cout << "d. Andi makan dan minum\n";
    cout << "Masukkan jawaban: ";
     cin >> jawaban;
      if(jawaban == 'a') scorePU += 100/3;
    cout << "=== Pemahaman dan Pengetahuan Umum ===\n";
    cout << "1. Jika p maka q. Andi melakukan p. Apa kesimpulan yang valid?\n";
    cout << "a. Andi makan\n";
    cout << "b. Andi minum\n";
    cout << "c. Andi makan dan minum\n";
    cout << "d. Andi q\n";
    cout << "Masukkan jawaban: ";
     cin >> jawaban;
      if(jawaban == 'd') scorePU += 100/3;
    cout << "2. Jika p maka q. Andi melakukan p. Apa kesimpulan yang valid?\n";
    cout << "a. Andi makan\n";
    cout << "b. Andi q\n";
    cout << "c. Andi minum\n";
    cout << "d. Andi makan dan minum\n";
    cout << "Masukkan jawaban: ";
     cin >> jawaban;
      if(jawaban == 'd') scorePU += 100/3;
    cout << "3. Jika p maka q. Andi melakukan p. Apa kesimpulan yang valid?\n";
    cout << "a. Andi q\n";
    cout << "b. Andi makan\n";
    cout << "c. Andi minum\n";
    cout << "d. Andi makan dan minum\n";
    cout << "Masukkan jawaban: ";
     cin >> jawaban;
      if(jawaban == 'a') scorePU += 100/3;
    cout << "=== Pemahaman Baca Tulis ===\n";
    cout << "1. Jika p maka q. Andi melakukan p. Apa kesimpulan yang valid?\n";
    cout << "a. Andi makan\n";
    cout << "b. Andi minum\n";
    cout << "c. Andi makan dan minum\n";
    cout << "d. Andi q\n";
    cout << "Masukkan jawaban: ";
     cin >> jawaban;
      if(jawaban == 'd') scorePU += 100/3;
    cout << "2. Jika p maka q. Andi melakukan p. Apa kesimpulan yang valid?\n";
    cout << "a. Andi makan\n";
    cout << "b. Andi q\n";
    cout << "c. Andi minum\n";
    cout << "d. Andi makan dan minum\n";
    cout << "Masukkan jawaban: ";
     cin >> jawaban;
      if(jawaban == 'd') scorePU += 100/3;
    cout << "3. Jika p maka q. Andi melakukan p. Apa kesimpulan yang valid?\n";
    cout << "a. Andi q\n";
    cout << "b. Andi makan\n";
    cout << "c. Andi minum\n";
    cout << "d. Andi makan dan minum\n";
    cout << "Masukkan jawaban: ";
     cin >> jawaban;
      if(jawaban == 'a') scorePU += 100/3;
    cout << "=== Pengetahuan Kuantitatif ===\n";
    cout << "1. Jika p maka q. Andi melakukan p. Apa kesimpulan yang valid?\n";
    cout << "a. Andi makan\n";
    cout << "b. Andi minum\n";
    cout << "c. Andi makan dan minum\n";
    cout << "d. Andi q\n";
    cout << "Masukkan jawaban: ";
     cin >> jawaban;
      if(jawaban == 'd') scorePU += 100/3;
    cout << "2. Jika p maka q. Andi melakukan p. Apa kesimpulan yang valid?\n";
    cout << "a. Andi makan\n";
    cout << "b. Andi q\n";
    cout << "c. Andi minum\n";
    cout << "d. Andi makan dan minum\n";
    cout << "Masukkan jawaban: ";
     cin >> jawaban;
      if(jawaban == 'd') scorePU += 100/3;
    cout << "3. Jika p maka q. Andi melakukan p. Apa kesimpulan yang valid?\n";
    cout << "a. Andi q\n";
    cout << "b. Andi makan\n";
    cout << "c. Andi minum\n";
    cout << "d. Andi makan dan minum\n";
    cout << "Masukkan jawaban: ";
     cin >> jawaban;
      if(jawaban == 'a') scorePU += 100/3;
    cout << "=== Literasi Bahasa Indonesia ===\n";
    cout << "1. Jika p maka q. Andi melakukan p. Apa kesimpulan yang valid?\n";
    cout << "a. Andi makan\n";
    cout << "b. Andi minum\n";
    cout << "c. Andi makan dan minum\n";
    cout << "d. Andi q\n";
    cout << "Masukkan jawaban: ";
     cin >> jawaban;
      if(jawaban == 'd') scorePU += 100/3;
    cout << "2. Jika p maka q. Andi melakukan p. Apa kesimpulan yang valid?\n";
    cout << "a. Andi makan\n";
    cout << "b. Andi q\n";
    cout << "c. Andi minum\n";
    cout << "d. Andi makan dan minum\n";
    cout << "Masukkan jawaban: ";
     cin >> jawaban;
      if(jawaban == 'd') scorePU += 100/3;
    cout << "3. Jika p maka q. Andi melakukan p. Apa kesimpulan yang valid?\n";
    cout << "a. Andi q\n";
    cout << "b. Andi makan\n";
    cout << "c. Andi minum\n";
    cout << "d. Andi makan dan minum\n";
    cout << "Masukkan jawaban: ";
     cin >> jawaban;
      if(jawaban == 'a') scorePU += 100/3;
    cout << "=== Literasi Bahasa Inggris ===\n";
    cout << "1. Jika p maka q. Andi melakukan p. Apa kesimpulan yang valid?\n";
    cout << "a. Andi makan\n";
    cout << "b. Andi minum\n";
    cout << "c. Andi makan dan minum\n";
    cout << "d. Andi q\n";
    cout << "Masukkan jawaban: ";
     cin >> jawaban;
      if(jawaban == 'd') scorePU += 100/3;
    cout << "2. Jika p maka q. Andi melakukan p. Apa kesimpulan yang valid?\n";
    cout << "a. Andi makan\n";
    cout << "b. Andi q\n";
    cout << "c. Andi minum\n";
    cout << "d. Andi makan dan minum\n";
    cout << "Masukkan jawaban: ";
     cin >> jawaban;
      if(jawaban == 'd') scorePU += 100/3;
    cout << "3. Jika p maka q. Andi melakukan p. Apa kesimpulan yang valid?\n";
    cout << "a. Andi q\n";
    cout << "b. Andi makan\n";
    cout << "c. Andi minum\n";
    cout << "d. Andi makan dan minum\n";
    cout << "Masukkan jawaban: ";
     cin >> jawaban;
      if(jawaban == 'a') scorePU += 100/3;
    cout << "=== Penalaran Matematika ===\n";
    cout << "1. Jika p maka q. Andi melakukan p. Apa kesimpulan yang valid?\n";
    cout << "a. Andi makan\n";
    cout << "b. Andi minum\n";
    cout << "c. Andi makan dan minum\n";
    cout << "d. Andi q\n";
    cout << "Masukkan jawaban: ";
     cin >> jawaban;
      if(jawaban == 'd') scorePU += 100/3;
    cout << "2. Jika p maka q. Andi melakukan p. Apa kesimpulan yang valid?\n";
    cout << "a. Andi makan\n";
    cout << "b. Andi q\n";
    cout << "c. Andi minum\n";
    cout << "d. Andi makan dan minum\n";
    cout << "Masukkan jawaban: ";
     cin >> jawaban;
      if(jawaban == 'd') scorePU += 100/3;
    cout << "3. Jika p maka q. Andi melakukan p. Apa kesimpulan yang valid?\n";
    cout << "a. Andi q\n";
    cout << "b. Andi makan\n";
    cout << "c. Andi minum\n";
    cout << "d. Andi makan dan minum\n";
    cout << "Masukkan jawaban: ";
    cin >> jawaban;
    if(jawaban == 'a') scorePU += 100/3;
    cout << "\nTerima kasih telah mengerjakan soal UTBK\n";
    double totalScore = scorePU * 10 / 7;
    Table tab = db.getTable("hasil_utbk");
    tab.insert("id_user", "subtes", "skor")
        .values(userId, "All Subtes", totalScore)
        .execute();

}

void announcement(int userId, std::string type){
    std::string hasil;
    if(type == "SNBT"){
        double score = data<double>("hasil_utbk", "skor", "id_user", userId);
        if(score > 500){
            cout << "Selamat Anda lolos " << type << "!\n";
            hasil = "LULUS";
        } else {
            cout << "Maaf Anda tidak lolos " << type << endl;
            cout << "Tetap semangat dan jangan menyerah!\n";
            hasil = "TIDAK_LULUS";
        }
        Table tab = db.getTable("pengumuman");
        tab.insert("id_user", "jalur_seleksi", "hasil_akhir")
            .values(userId, type, hasil)
            .execute();
    } else if(type == "SNBP"){
        int biodataId = data<int>("biodata", "id_biodata", "id_user", userId);
        double score = data<double>("nilai_rapor_snbp", "avg(rata_rata)", "id_biodata", biodataId);
        if(score > 85){
            cout << "Selamat Anda lolos " << type << "!\n";
            hasil = "LULUS";
        } else {
            cout << "Maaf Anda tidak lolos " << type << endl;
            cout << "Tetap semangat dan jangan menyerah!\n";
            hasil = "TIDAK_LULUS";
        }
        Table tab = db.getTable("pengumuman");
        tab.insert("id_user", "jalur_seleksi", "hasil_akhir")
            .values(userId, type, hasil)
            .execute();
    }
    
}

int main() {
    Table tab = db.getTable("akun_user");
    int option, userId, biodataId, nisn;
    std::string username, isEligible, result;

    userId = login();
    bool reInput = true, null, exist;
    char confirm;

    do{
        cout << "\n=== MENU UTAMA ===\n";
        cout << "1. Verifikasi Data Siswa dan Sekolah\n";
        cout << "2. Pendaftaran SNBP\n";
        cout << "3. Pendaftaran SNBT\n";
        cout << "4. Exit\n";
        cout << "Choose [1-4]: ";
        cin >> option;
        
        int nisn, tahunLulus, sekolahId;
        std::string nama, temp, tempat, tanggalLahir, sekolah, jurusan;
        switch(option){
            case 1:
                cout << "\n=== Verifikasi biodata ===\n";
                null = isNull("biodata", "nama_lengkap", "id_user", userId);
                if(!null){
                    sess.sql("USE snpmb").execute();
                    SqlResult sqlRes = sess.sql("SELECT b.nisn, b.nama_lengkap, b.tempat_lahir, b.tanggal_lahir, b.jurusan, b.tahun_lulus, sa.nama_sekolah "
                                                "FROM biodata b "
                                                "JOIN sekolah_asal sa ON b.id_sekolah = sa.id_sekolah "
                                                "JOIN akun_user au ON au.id_user = b.id_user "
                                                "WHERE au.id_user = " + std::to_string(userId))
                                                .execute();
                    Row row;
                        
                    username = data<std::string>("biodata", "nama_lengkap", "id_user", userId);
                    cout << "Your data has been verified!\n\n";
                    cout << "Welcome " << username << endl;
                    while((row = sqlRes.fetchOne())){
                        cout << "NISN: " << row[0] << endl;
                        cout << "Nama: " << row[1] << endl;
                        cout << "Tempat Lahir: " << row[2] << endl;
                        cout << "Tanggal Lahir: " << row[3] << endl;
                        cout << "Jurusan: " << row[4] << endl;
                        cout << "Tahun Lulus: " << row[5] << endl;
                        cout << "Sekolah: " << row[6] << endl;
                    }
                } else {
                    cin.ignore();
                    cout << "Nama: ";
                    getline(cin, nama);
                    // cin.ignore();
                    cout << "Tempat lahir: ";
                    getline(cin, tempat);
                    cout << "Tanggal Lahir [YYYY-MM-DD]: ";
                    cin >> tanggalLahir;
                    cin.ignore();
                    cout << "Sekolah: ";
                    getline(cin, sekolah);
                    // cout << sekolah << endl;
                    // cin.ignore();
                    cout << "Jurusan: ";
                    getline(cin, jurusan);
                    cout << "Tahun lulus: ";
                    cin >> tahunLulus;
                    
                    sekolahId = data<int>("sekolah_asal", "id_sekolah", "nama_sekolah", sekolah);
                    tab = db.getTable("biodata");

                    tab.update().set("nama_lengkap", nama).set("tempat_lahir", tempat).set("tanggal_lahir", tanggalLahir)
                                .set("jurusan", jurusan).set("tahun_lulus", tahunLulus).set("id_sekolah", sekolahId)
                                .where("id_user = :param")
                                .bind("param", userId)
                                .execute();

                    cout << "Data updated successfully!\n";
                }
                break;
            case 2:
                // if nisn is not in the isEligible database, print you are not eligible 
                null = isNull("biodata", "nama_lengkap", "id_user", userId);
                if(null){
                    cout << "\nYou have to verify your data first!\n";
                    break;
                }

                isEligible = data<std::string>("biodata", "status_eligible", "id_user", userId);
    
                if(isEligible == "Tidak_Eligible"){
                    cout << "Maaf anda bukan siswa eligible.\n";
                    break;
                }

                exist = isExist("pengumuman", "hasil_akhir", "id_user", userId);
                if(exist){
                    result = data<std::string>("pengumuman", "hasil_akhir", "id_user", userId);
                    if(result == "LULUS"){
                        cout << "Selamat anda LULUS SNBP\n";
                        break;
                    } else {
                        cout << "Anda dinyatakan TIDAK LULUS SNBP\n";
                        cout << "Tetap semangat dan jangan menyerah\n";
                        break;
                    }
                }

                cout << "\n=== SNBP ===\n";
                biodataId = data<int>("biodata", "id_biodata", "id_user", userId);
                inputJurusan("SNBP", userId, biodataId);
                announcement(userId, "SNBP");
            

                break;
            case 3:
                null = isNull("biodata", "nama_lengkap", "id_user", userId);
                if(null){
                    cout << "\nYou have to verify your data first!\n";
                    break;
                }

                exist = isExist("pengumuman", "hasil_akhir", "id_user", userId);
                if(exist){
                    result = data<std::string>("pengumuman", "hasil_akhir", "id_user", userId);
                    if(result == "LULUS"){
                        cout << "Selamat anda LULUS UTBK\n";
                        break;
                    } else {
                        cout << "Anda dinyatakan TIDAK LULUS UTBK\n";
                        cout << "Tetap semangat dan jangan menyerah\n";
                        break;
                    }
                }

                cout << "\n=== SNBT ===\n";
                biodataId = data<int>("biodata", "id_biodata", "id_user", userId);
                inputJurusan("SNBT", userId, biodataId);
                soalUtbk(userId);
                announcement(userId, "SNBT");
                break;
            case 4:
                cout << "BYE\n";
                break;
            default:
                cout << "Invalid Input! Masukkan angka [1-4]\n";
            break;
        }
    }while(option != 4);
    
    sess.close();
    return 0;
}