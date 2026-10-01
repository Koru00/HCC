#include <HolyTest/Core/TestHarness.h>

#include <HolyCore/StringInterner.h>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

using namespace holy::core;

TEST(Interner_FreshState) {
	Arena a(1 << 20);
	StringInterner in(a);
	
	CHECK(in.Count() == 0);
	CHECK(!in.Find("x").IsValid());
}

TEST(Interner_SameTextSameAtom) {
	Arena a(1 << 20);
	StringInterner in(a);
	std::string foo1 = "foo";
	std::string foo2 = "foo";
	in.Intern(foo1);
	in.Intern(foo2);
	CHECK(in.Find(foo1) == in.Find(foo2));
	CHECK(in.Count() == 1);
	CHECK(in.Text(in.Find(foo1)).data() == in.Text(in.Find(foo2)).data());
}

TEST(Interner_DifferentTexts) {
	Arena a(1 << 20);
	StringInterner in(a);
	std::string foo = "foo";
	std::string bar = "bar";
	std::string fo = "fo";
	std::string foobar = "foobar";
	in.Intern(foo);
	in.Intern(bar);
	in.Intern(fo);
	in.Intern(foobar);
	CHECK(in.Find(foo) != in.Find(bar));
	CHECK(in.Find(fo) != in.Find(foobar));
}

TEST(Interner_EmptyString) {
	Arena a(1 << 20);
	StringInterner in(a);
	in.Intern("");
	CHECK(in.Find("").IsValid());
	CHECK(in.Text(in.Find("")).empty());
	CHECK(in.Intern(std::string_view{}) == in.Intern(""));
	CHECK(in.Intern("a") != in.Intern(""));
	CHECK(in.Count() == 2);
}

TEST(Interner_StableAndNulTerminated) {
	Arena a(1 << 20);
	StringInterner in(a);
	std::vector<Atom> atoms;
	atoms.resize(5000);
	for (int i = 0; i < 5000; i++) {
		atoms[i] = in.Intern("id_" + std::to_string(i));
	}

	const char* p = in.Text(atoms[0]).data();
	for (int i = 0; i < 5000; i++) {
		CHECK(in.Text(atoms[i]).compare("id_" + std::to_string(i)) == 0);
		CHECK(in.Text(atoms[i]).data()[in.Text(atoms[i]).size()] == '\0');
	}

	CHECK(p == in.Text(atoms[0]).data());
	CHECK(in.Count() == 5000);
	CHECK(in.Intern("id_1234") == atoms[1234]);
}

TEST(Interner_FindDoesNotInsert) {
	Arena a(1 << 20);
	StringInterner in(a);
	CHECK(!in.Find("nope").IsValid());
	CHECK(in.Count() == 0);
	Atom atom = in.Intern("yes");
	CHECK(atom == in.Find("yes"));
	CHECK(in.Count() == 1);
}

TEST(Interner_AtomAsMapKey) {
	Arena a(1 << 20);
	HashMap<Atom, int> m(a);
	Atom atom1 = Atom{ 1 };
	Atom atom2 = Atom{ 2 };
	Atom atom3 = Atom{ 3 };
	m.Insert(atom1, 10);
	m.Insert(atom2, 20);
	m.Insert(atom3, 30);
	CHECK(*m.Find(atom1) == 10);
	CHECK(*m.Find(atom2) == 20);
	CHECK(*m.Find(atom3) == 30);
	CHECK(m.Find(Atom{}) == nullptr);
	CHECK(DefaultHash<Atom>()(atom1) == DefaultHash<Atom>()(atom1));
	CHECK(DefaultHash<Atom>()(atom1) != DefaultHash<Atom>()(atom2));
}

TEST(Interner_EmbeddedNul) {
	Arena a(1 << 20);
	StringInterner in(a);
	std::string_view x("a\0b", 3);
	std::string_view y("a\0c", 3);
	in.Intern(x);
	in.Intern(y);
	CHECK(in.Find(x) != in.Find(y));
	CHECK(in.Text(in.Find(x)).size() == 3);
	CHECK(in.Text(in.Find(x)) == x);
}