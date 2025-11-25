#include <mysqlx/xdevapi.h>
#include <iostream>
#include <vector>
using namespace mysqlx;
using namespace std;

Session sess("localhost", 33060, "root", "");
Schema db = sess.getSchema("snpmb");
RowResult res;
DocResult docs;
Row row;

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

int login(){
    int option, userId;
    std::string username, password, email, tanggalLahir, temp;
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
        cout << "=== LOGIN ===\n";
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
        cout << "=== REGISTER ===\n";
        cout << "Email: ";
        cin >> email;
        cout << "Create username: ";
        cin.ignore();
        getline(cin, username);
        cout << "Create password: ";
        cin >> password;
        cout << "Tanggal lahir [YYYY-MM-DD]: ";
        cin >> tanggalLahir;

        exist = isExist("akun_user", "email", "email", email);
        if(exist){
            cout << "Email is already registered!\n";
            goto loginRegister;
        }

        tab.insert("email", "nama_user", "password", "tanggal_lahir")
            .values(email, username, password, tanggalLahir)
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

// void inputJurusan(){
//     Table tab = db.getTable();
//     vector<std::string> ptn;
//     vector<std::string> jurusan;
//     bool reInput = true;
//     int jumlahJurusan;
//     char confirm;

//     while(reInput){
//         cout << "Masukkan jumlah jurusan (1-4): ";
//         cin >> jumlahJurusan;
//         std::string temp;

//         for(int i = 0; i < jumlahJurusan; i++){
//             cout << "PTN Pilihan " << i + 1 << ": ";
//             cin >> temp;
//             ptn.push_back(temp);
//                         // sql query
//             cout << "Program Studi Pilihan " << i + 1 << ": ";
//             cin >> temp;
//             jurusan.push_back(temp);
//         }

//         cout << "Ketik [Y/y] jika sudah benar \n => ";
//         cin >> confirm;

//         if(confirm == 'Y' || confirm == 'y'){
//             // insert
//             reInput = false;
//         } else {
//             ptn.clear();
//             jurusan.clear();
//         }
//     }
// }

int main() {
    Table tab = db.getTable("akun_user");
    int option, userId;
    std::string username;

    userId = login();
    bool reInput = true, exist;
    char confirm;
    
    do{
        cout << "=== MENU UTAMA ===\n";
        cout << "1. Verifikasi Data Siswa dan Sekolah\n";
        cout << "2. Pendaftaran SNBP\n";
        cout << "3. Pendaftaran UTBK-SNBT\n";
        cout << "4. Exit\n";
        cout << "Choose [1-4]: ";
        cin >> option;
        
        int nisn, tahunLulus, sekolahId;
        std::string nama, tempat, tanggalLahir, sekolah, jurusan;
        switch(option){
            case 1:
            cout << "=== Verifikasi biodata ===\n";

            exist = isExist("biodata", "id_user", "id_user", userId);
            if(exist){
                Table tab = db.getTable("biodata");
                RowResult res = tab.select("id_biodata", "id_user", "id_sekolah", "nama_lengkap")
                            .where("id_user = :param")
                            .bind("param", userId)
                            .execute();

                cout << "Your data: \n";
                Row row;
                
                username = data<std::string>("biodata", "nama_lengkap", "id_user", userId);
                cout << "\nWelcome " << username << endl;
                cout << "Your data has been verified!\n";
                while((row = res.fetchOne())){
                    cout << "Id biodata: " << row[0] << endl;
                    cout << "Id user: " << row[1] << endl;
                    cout << "Id sekolah: " << row[2] << endl;
                    cout << "Nama: " << row[3] << endl << endl;
                }
            } else {
                cout << "NISN: ";
                cin >> nisn;
                cout << "Nama: ";
                cin.ignore();
                getline(cin, nama);
                cout << "Tempat lahir: ";
                cin.ignore();
                getline(cin, tempat);
                cout << "Tanggal Lahir [YYYY-MM-DD]: ";
                cin >> tanggalLahir;
                cout << "Sekolah: ";
                cin.ignore();
                getline(cin, sekolah);
                cout << "Jurusan: ";
                cin.ignore();
                getline(cin, jurusan);
                cout << "Tahun lulus: ";
                cin >> tahunLulus;
                
                sekolahId = data<int>("sekolah_asal", "id_sekolah", "nama_sekolah", sekolah);
                tab = db.getTable("biodata");
                tab.insert("id_user", "id_sekolah", "nisn", "nama_lengkap", "tempat_lahir", "tanggal_lahir", "jurusan", "tahun_lulus")
                    .values(userId, sekolahId, nisn, nama, tempat, tanggalLahir, jurusan, tahunLulus)
                    .execute();

                cout << "Data updated successfully!\n";
            }
            break;
            case 2:
                // if nisn is not in the isEligible database, print you are not eligible 
            cout << "=== SNBP ===\n";

            break;
            case 3:

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